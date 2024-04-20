//
// Created by mizzh on 4/15/2024.
//

#include "connection.h"

#include <asio/thread_pool.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/placeholders.hpp>
#include <asio/read.hpp>
#include <asio/redirect_error.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/write.hpp>

#include <fmt/format.h>

#include "util/asio.h"
#include "util/make.h"
#include "message/payload.h"

#include "logger.h"

namespace ar
{
  Connection::Connection(asio::any_io_executor& executor) noexcept :
    m_send_timer{executor, std::chrono::steady_clock::time_point::max()}, m_socket{executor}
  {
  }

  Connection::Connection(Connection&& other) noexcept
    : m_send_timer{std::move(other.m_send_timer)}, m_write_buffer{std::move(other.m_write_buffer)},
      m_socket{std::move(other.socket())}, m_user{std::move(other.m_user)}
  {
  }

  Connection& Connection::operator=(Connection&& other) noexcept
  {
    if (this == &other)
      return *this;

    m_send_timer = std::move(other.m_send_timer);
    m_write_buffer = std::move(other.m_write_buffer);
    m_socket = std::move(other.m_socket);
    m_user = std::move(other.m_user);
    return *this;
  }

  Connection::~Connection() noexcept
  {
  }

  asio::awaitable<asio::error_code> Connection::connect(const asio::ip::tcp::endpoint& endpoint) noexcept
  {
    asio::error_code ec;
    try
    {
      co_await m_socket.async_connect(endpoint, ar::await_with_error(ec));
    }
    catch (std::exception& exc)
    {
      fmt::println("{}", exc.what());
    }
    co_return ec;
  }

  void Connection::start() noexcept
  {
    Logger::info("Connection started!");
    asio::co_spawn(m_socket.get_executor(), [this] { return write_handler(); }, asio::detached);
  }

  void Connection::close() noexcept
  {
    if (m_is_closing.load())
      return;
    m_is_closing.store(true);
    m_send_timer.cancel();
    m_socket.cancel();
    m_socket.close();
    Logger::info("connection closed!");
  }

  asio::awaitable<std::expected<Message, asio::error_code>> Connection::read() noexcept
  {
    Logger::trace("Connection waiting new message from remote...");
    Message message{};
    {
      auto [ec, n] = co_await asio::async_read(m_socket, asio::buffer(message.header),
                                               asio::transfer_exactly(Message::header_size), ar::await_with_error());
      if (ec || n != Message::header_size)
        co_return std::unexpected(ec);
    }

    auto header = message.get_header();
    message.body.reserve(header->body_size);
    auto [ec, n] = co_await asio::async_read(m_socket, asio::dynamic_buffer(message.body, header->body_size),
                                             ar::await_with_error());
    if (ec || n != header->body_size)
      co_return std::unexpected(ec);

    Logger::info(fmt::format("Connection got data {} bytes", message.size()));
    co_return ar::make_expected<Message, asio::error_code>(std::move(message));
  }

  bool Connection::is_open() const noexcept
  {
    return m_socket.is_open() && !m_is_closing.load();
  }

  void Connection::write(Message&& msg) noexcept
  {
    if (!is_open())
    {
      Logger::warn("Could not send data due to remote connection already closed");
      return;
    }

    Logger::trace(fmt::format("Connection send data {} bytes", msg.size()));
    m_write_buffer.push(std::forward<Message>(msg));
    asio::error_code ec;
    m_send_timer.cancel_one(ec);
    if constexpr (_DEBUG)
    {
      if (ec)
        Logger::warn(fmt::format("failed to cancel timer: {}", ec.message()));
    }
  }

  asio::awaitable<void> Connection::write_handler() noexcept
  {
    while (is_open())
    {
      if (m_write_buffer.empty())
      {
        Logger::trace("connection waiting for new message to write");
        auto [ec] = co_await m_send_timer.async_wait(ar::await_with_error());
        if (ec == asio::error::timed_out || ec == asio::error::operation_aborted)
          continue;
        break;
      }
      auto& msg = m_write_buffer.front();
      {
        Logger::trace("connection sending message...");
        auto [ec, n] = co_await asio::async_write(m_socket, asio::buffer(msg.header), ar::await_with_error());
        if (ec || n != Message::header_size)
        {
          Logger::warn("connection failed to send payload header");
          break;
        }
      }
      {
        auto [ec, n] = co_await asio::async_write(m_socket, asio::buffer(msg.body), ar::await_with_error());
        if (ec || n != msg.body.size())
        {
          Logger::warn("connection failed to send payload body");
          break;
        }
      }
      Logger::info("connection sent message!");
      m_write_buffer.pop();
    }
    Logger::trace("connection no longer run write handler");
    // close(); TODO: Should be called from outside class
  }
} // namespace ar
