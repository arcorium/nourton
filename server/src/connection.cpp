//
// Created by mizzh on 4/19/2024.
//

#include "connection.h"

#include <fmt/format.h>

#include <util/asio.h>

#include "handler.h"
#include "logger.h"

#include "message/payload.h"

namespace ar
{
  Connection::Connection(asio::ip::tcp::socket&& socket, IMessageHandler* message_handler,
                         IConnectionHandler* connection_handler) noexcept
    : m_message_handler{message_handler}, m_connection_handler{connection_handler}, m_id{s_current_id.fetch_add(1)},
      m_user{}, m_is_closing{false}, m_write_timer{socket.get_executor(), std::chrono::steady_clock::time_point::max()},
      m_socket{std::forward<decltype(socket)>(socket)}
  {
  }

  Connection::~Connection() noexcept
  {
    Logger::trace(fmt::format("Connection-{} deconstructed", m_id));
    // close();
  }

  Connection::Connection(Connection&& other) noexcept: m_message_handler(other.m_message_handler),
                                                       m_connection_handler(other.m_connection_handler),
                                                       m_id(other.m_id),
                                                       m_user(other.m_user),
                                                       m_is_closing(other.m_is_closing.exchange(true)),
                                                       m_write_timer(std::move(other.m_write_timer)),
                                                       m_write_buffers(std::move(other.m_write_buffers)),
                                                       m_socket(std::move(other.m_socket))
  {
    other.m_id = std::numeric_limits<id_type>::max();
    other.m_user = nullptr;
    other.m_connection_handler = nullptr;
    other.m_message_handler = nullptr;
  }

  Connection& Connection::operator=(Connection&& other) noexcept
  {
    if (this == &other)
      return *this;
    m_message_handler = other.m_message_handler;
    m_connection_handler = other.m_connection_handler;
    m_id = other.m_id;
    m_user = other.m_user;
    m_is_closing = other.m_is_closing.exchange(true);
    m_write_timer = std::move(other.m_write_timer);
    m_write_buffers = std::move(other.m_write_buffers);
    m_socket = std::move(other.m_socket);


    other.m_id = std::numeric_limits<id_type>::max();
    other.m_user = nullptr;
    other.m_connection_handler = nullptr;
    other.m_message_handler = nullptr;
    return *this;
  }

  std::shared_ptr<Connection> Connection::make_shared(asio::ip::tcp::socket&& socket, IMessageHandler* message_handler,
                                                      IConnectionHandler* connection_handler) noexcept
  {
    return std::make_shared<Connection>(std::forward<decltype(socket)>(socket), message_handler, connection_handler);
  }

  void Connection::start() noexcept
  {
    Logger::info(fmt::format("Connection-{} started!", m_id));
    asio::co_spawn(m_socket.get_executor(), [self=shared_from_this()] { return self->reader(); }, asio::detached);
    asio::co_spawn(m_socket.get_executor(), [self=shared_from_this()] { return self->writer(); }, asio::detached);
  }

  void Connection::close() noexcept
  {
    Logger::info(fmt::format("trying to close connection-{}", m_id));
    if (m_is_closing.load())
      return;
    m_is_closing.store(true);
    m_write_timer.cancel();
    m_socket.cancel();
    m_socket.close();
    Logger::info(fmt::format("Connection-{} closed!", m_id));
  }

  void Connection::write(Message&& msg) noexcept
  {
    if (!is_open())
    {
      Logger::warn(fmt::format("Could not send data to connection-{} because the socket already closed", m_id));
      return;
    }

    Logger::trace(fmt::format("Sending message to connection-{} ...", m_id));
    m_write_buffers.push(std::forward<Message>(msg));

    asio::error_code ec;
    m_write_timer.cancel(ec);

    if constexpr (_DEBUG)
    {
      if (ec)
        Logger::warn(fmt::format("failed to cancel timer: {}", ec.message()));
    }
  }

  bool Connection::is_open() const noexcept
  {
    return m_socket.is_open() && !m_is_closing.load();
  }

  bool Connection::is_authenticated() const noexcept
  {
    return m_user;
  }

  void Connection::user(User* user) noexcept
  {
    m_user = user;
  }

  asio::awaitable<void> Connection::reader() noexcept
  {
    Logger::trace(fmt::format("Connection-{} waiting new message", m_id));
    Message message{};
    while (is_open())
    {
      {
        auto [ec, n] = co_await asio::async_read(m_socket, asio::buffer(message.header),
                                                 asio::transfer_exactly(Message::header_size), ar::await_with_error());
        if (ec)
        {
          if (is_connection_lost(ec))
            break;
          Logger::warn(fmt::format("Connection-{} error on reading header: {}", m_id, ec.message()));
          continue;
        }
        if (n != Message::header_size)
        {
          Logger::warn(fmt::format("Connection-{} is reading header with different size", m_id));
          continue;
        }
      }
      {
        // TODO: Use transfer exactly with body size?
        auto header = message.as_header();
        message.body.reserve(header->body_size);
        auto [ec, n] = co_await asio::async_read(m_socket, asio::buffer(message.body),
                                                 asio::transfer_exactly(header->body_size),
                                                 ar::await_with_error());
        // if (n != header->body_size)
        // {
        //   Logger::warn(fmt::format("Connection-{} is reading body with different size", m_id));
        //   continue;
        // }
        if (ec)
        {
          if (is_connection_lost(ec))
            break;
          Logger::warn(fmt::format("Connection-{} error on reading body: {}", m_id, ec.message()));
          continue;
        }
      }
      Logger::info(fmt::format("Connection-{} got data {} bytes", m_id, message.size()));
      if (m_message_handler)
        m_message_handler->on_message_in(*this, message);
    }
    Logger::trace(fmt::format("Connection-{} no longer reading message!", m_id));
    close();
    if (m_connection_handler)
      m_connection_handler->on_connection_closed(*this);
  }

  asio::awaitable<void> Connection::writer() noexcept
  {
    Logger::trace(fmt::format("Connection-{} starting writer handler", m_id));
    while (is_open())
    {
      if (m_write_buffers.empty())
      {
        auto [ec] = co_await m_write_timer.async_wait(ar::await_with_error());
        // NOTE: when the error is not cancel
        if (ec != asio::error::operation_aborted)
        {
          Logger::warn(fmt::format("Error while waiting timer: {}", ec.message()));
          break;
        }
        continue;
      }

      auto msg = std::move(m_write_buffers.front());
      {
        auto [ec, n] = co_await asio::async_write(m_socket, asio::buffer(msg.header), ar::await_with_error());
        if (ec)
        {
          if (is_connection_lost(ec))
            break;
          Logger::warn(fmt::format("Connection-{} error on sending message header: {}", m_id, ec.message()));
          continue;
        }
        if (n != Message::header_size)
        {
          Logger::warn(fmt::format("Connection-{} is sending message header with different size: {}", m_id, n));
          continue;
        }
      }
      {
        auto [ec, n] = co_await asio::async_write(m_socket, asio::dynamic_buffer(msg.body), ar::await_with_error());
        if (ec)
        {
          if (is_connection_lost(ec))
            break;
          Logger::warn(fmt::format("Connection-{} error on sending message body: {}", m_id, ec.message()));
          continue;
        }
        if (n != msg.as_header()->body_size)
        {
          Logger::warn(fmt::format("Connection-{} is sending message body with different size: {}", m_id, n));
          continue;
        }
      }

      Logger::info(fmt::format("success sent 1 message to connection-{}!", m_id));
      m_write_buffers.pop();
    }
    Logger::trace(fmt::format("Connection-{} no longer sending message!", m_id));
    // if (m_connection_handler)
    // m_connection_handler->on_connection_closed(*this); // TODO: Change this, when the connection removed from connection_handler, the class itself could be still used on some async actions
  }
} // ar
