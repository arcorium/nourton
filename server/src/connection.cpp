//
// Created by mizzh on 4/15/2024.
//

#include "connection.h"

#include <asio/placeholders.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>
#include <fmt/format.h>

#include "logger.h"

namespace ar
{
  Connection::Connection(asio::ip::tcp::socket&& socket, IMessageHandler& message_handler,
                         IConnectionHandler& connection_handler) noexcept :
    m_message_handler{message_handler}, m_connection_handler{connection_handler}, m_id{s_current_id.fetch_add(1)},
    m_is_writing{false}, m_socket{std::forward<decltype(socket)>(socket)}
  {
  }

  Connection::~Connection() noexcept
  {
    stop();
  }

  std::unique_ptr<Connection> Connection::make_unique(asio::ip::tcp::socket&& socket,
                                                      IMessageHandler& message_handler,
                                                      IConnectionHandler& connection_handler) noexcept
  {
    return std::make_unique<Connection>(std::forward<decltype(socket)>(socket), message_handler, connection_handler);
  }

  void Connection::start() noexcept
  {
    asio::post(m_socket.get_executor(), [this] {
      Logger::trace(fmt::format("connection-{} started!", m_id));
      read_header();
    });
  }

  void Connection::user(User* user) noexcept
  {
    m_user = user;
  }

  void Connection::stop() noexcept
  {
    m_socket.cancel();
    m_socket.close();
    Logger::info(fmt::format("connection-{} closed!", m_id));
  }

  void Connection::read_header() noexcept
  {
    Logger::trace(fmt::format("client-{}", m_id));
    asio::async_read(m_socket, asio::buffer(m_input_message.header),
                     asio::transfer_at_least(Message::size),
                     std::bind(&Connection::read_header_handler, this,
                               asio::placeholders::error,
                               asio::placeholders::bytes_transferred));
  }

  void Connection::read_body() noexcept
  {
    Logger::trace(fmt::format("client-{}", m_id));
    // message body already set by read handler
    asio::async_read(m_socket, asio::buffer(m_input_message.body),
                     asio::transfer_at_least(Message::size),
                     std::bind(&Connection::read_body_handler, this,
                               asio::placeholders::error,
                               asio::placeholders::bytes_transferred));
  }

  void Connection::write(const Message& msg) noexcept
  {
    Logger::trace(fmt::format("client-{}", m_id));
    // send header and body in separate but sequentially
    m_write_buffer.emplace(msg.header.begin(), msg.header.end());
    m_write_buffer.emplace(msg.body);

    if (m_is_writing.load())
      return;

    m_is_writing.store(true);
    asio::async_write(m_socket, asio::dynamic_buffer(m_input_message.body),
                      std::bind(&Connection::write_handler, this, asio::placeholders::error,
                                asio::placeholders::bytes_transferred));
  }

  bool Connection::is_authenticated() const noexcept
  {
    return m_user != nullptr;
  }

  void Connection::close() noexcept
  {
    if (m_is_closing.load())
      return;
    m_is_closing.store(true);
    Logger::info(fmt::format("closing client-{}", m_id));

    asio::post(m_socket.get_executor(), [this] {
      m_connection_handler.on_connection_closed(*this);
    });
  }

  void Connection::read_header_handler(const asio::error_code& ec, usize n) noexcept
  {
    Logger::trace(fmt::format("client-{} get data {} bytes", m_id, n));
    if (ec)
    {
      if (ec == asio::error::eof)
      {
        close();
        return;
      }
      Logger::warn(fmt::format("client-{} got error: {}", m_id, ec.message()));
      read_header();
      return;
    }
    // Parse header
    auto header = m_input_message.parse_header();
    m_input_message.body.resize(header.body_size);
    read_body();
  }

  void Connection::read_body_handler(const asio::error_code& ec, usize n) noexcept
  {
    Logger::trace(fmt::format("client-{} reads {} bytes", m_id, n));
    if (ec)
    {
      if (ec == asio::error::eof)
      {
        close();
        return;
      }
      Logger::warn(fmt::format("client-{} got error: {}", m_id, ec.message()));
      read_header();
      return;
    }
    // Parse body
    m_message_handler.on_message_in(*this, m_input_message);

    read_header();
  }

  void Connection::write_handler(const asio::error_code& ec, usize n) noexcept
  {
    Logger::trace(fmt::format("client-{} writes {} bytes", m_id, n));
    if (ec)
    {
      if (ec == asio::error::eof)
      {
        close();
        return;
      }
      Logger::warn(fmt::format("client-{} got error: {}", m_id, ec.message()));
    }

    auto& front = m_write_buffer.front();
    m_message_handler.on_message_out(*this, front);
    m_write_buffer.pop();

    if (m_write_buffer.empty())
    {
      m_is_writing.store(false);
      return;
    }

    asio::async_write(m_socket, asio::dynamic_buffer(m_input_message.body),
                      std::bind(&Connection::write_handler, this, asio::placeholders::error,
                                asio::placeholders::bytes_transferred));
  }
} // namespace ar
