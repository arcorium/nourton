//
// Created by mizzh on 4/19/2024.
//

#include "connection.h"

#include <fmt/format.h>

#include <util/asio.h>

#include "handler.h"
#include "logger.h"

#include "message/payload.h"

namespace ar {
  Connection::Connection(asio::ip::tcp::socket &&socket, IMessageHandler *message_handler,
                         IConnectionHandler *connection_handler) noexcept
    : message_handler_{message_handler}, connection_handler_{connection_handler}, id_{s_current_id.fetch_add(1)},
      user_{}, is_closing_{false},
    // is_closing is false at the constructor due to the socket should be already connected
      write_timer_{socket.get_executor(), std::chrono::steady_clock::time_point::max()},
      socket_{std::forward<decltype(socket)>(socket)} {
  }

  Connection::~Connection() noexcept {
    Logger::trace(fmt::format("Connection-{} deconstructed", id_));
    // close();
  }

  Connection::Connection(Connection &&other) noexcept
    : message_handler_(other.message_handler_), connection_handler_(other.connection_handler_),
      id_(other.id_), user_(other.user_), is_closing_(other.is_closing_.exchange(true)),
      write_timer_(std::move(other.write_timer_)), write_message_queue_(std::move(other.write_message_queue_)),
      socket_(std::move(other.socket_)) {
    other.id_ = std::numeric_limits<id_type>::max();
    other.user_ = nullptr;
    other.connection_handler_ = nullptr;
    other.message_handler_ = nullptr;
  }

  Connection &Connection::operator=(Connection &&other) noexcept {
    if (this == &other)
      return *this;
    message_handler_ = other.message_handler_;
    connection_handler_ = other.connection_handler_;
    id_ = other.id_;
    user_ = other.user_;
    is_closing_ = other.is_closing_.exchange(true);
    write_timer_ = std::move(other.write_timer_);
    write_message_queue_ = std::move(other.write_message_queue_);
    socket_ = std::move(other.socket_);


    other.id_ = std::numeric_limits<id_type>::max();
    other.user_ = nullptr;
    other.connection_handler_ = nullptr;
    other.message_handler_ = nullptr;
    return *this;
  }

  std::shared_ptr<Connection> Connection::make_shared(asio::ip::tcp::socket &&socket, IMessageHandler *message_handler,
                                                      IConnectionHandler *connection_handler) noexcept {
    return std::make_shared<Connection>(std::forward<decltype(socket)>(socket), message_handler, connection_handler);
  }

  void Connection::start() noexcept {
    Logger::info(fmt::format("Connection-{} started!", id_));
    asio::co_spawn(socket_.get_executor(), [self = shared_from_this()] { return self->reader(); }, asio::detached);
    asio::co_spawn(socket_.get_executor(), [self = shared_from_this()] { return self->writer(); }, asio::detached);
  }

  void Connection::close() noexcept {
    Logger::info(fmt::format("trying to close connection-{}", id_));
    if (is_closing_.load())
      return;
    is_closing_.store(true);
    write_timer_.cancel();
    socket_.cancel();
    socket_.close();
    Logger::info(fmt::format("Connection-{} closed!", id_));
  }

  void Connection::write(Message &&msg) noexcept {
    if (!is_open()) {
      Logger::warn(fmt::format("Could not send data to connection-{} because the socket already closed", id_));
      return;
    }

    Logger::trace(fmt::format("Sending message to connection-{} ...", id_));
    write_message_queue_.push(std::forward<Message>(msg));

    asio::error_code ec;
    write_timer_.cancel(ec);

    if constexpr (_DEBUG) {
      if (ec)
        Logger::warn(fmt::format("failed to cancel timer: {}", ec.message()));
    }
  }

  bool Connection::is_open() const noexcept {
    return socket_.is_open() && !is_closing_.load();
  }

  bool Connection::is_authenticated() const noexcept {
    return user_;
  }

  void Connection::user(User *user) noexcept {
    user_ = user;
  }

  asio::awaitable<void> Connection::reader() noexcept {
    Logger::trace(fmt::format("Connection-{} waiting new message", id_));
    Message message{};
    while (is_open()) {
      {
        auto [ec, n] = co_await asio::async_read(socket_, asio::buffer(message.header),
                                                 asio::transfer_exactly(Message::header_size), ar::await_with_error());
        if (ec) {
          if (is_connection_lost(ec))
            break;
          Logger::warn(fmt::format("Connection-{} error on reading header: {}", id_, ec.message()));
          continue;
        }
        if (n != Message::header_size) {
          Logger::warn(fmt::format("Connection-{} is reading header with different size", id_));
          continue;
        }
      }
      {
        auto header = message.as_header();
        message.body.resize(header->body_size); // NOTE: need to resize instead of reserve because of using asio::buffer
        auto [ec, n] = co_await asio::async_read(socket_, asio::buffer(message.body),
                                                 asio::transfer_exactly(header->body_size),
                                                 ar::await_with_error());
        if (ec) {
          if (is_connection_lost(ec))
            break;
          Logger::warn(fmt::format("Connection-{} error on reading body: {}", id_, ec.message()));
          continue;
        }
        if (n != header->body_size) {
          Logger::warn(fmt::format("Connection-{} is reading body with different size", id_));
          continue;
        }
      }
      Logger::info(fmt::format("Connection-{} got data {} bytes", id_, message.size()));
      if (message_handler_)
        message_handler_->on_message_in(*this, message);
    }
    Logger::trace(fmt::format("Connection-{} no longer reading message!", id_));
    close();
    if (connection_handler_)
      connection_handler_->on_connection_closed(*this);
  }

  asio::awaitable<void> Connection::writer() noexcept {
    Logger::trace(fmt::format("Connection-{} starting writer handler", id_));
    while (is_open()) {
      if (write_message_queue_.empty()) {
        auto [ec] = co_await write_timer_.async_wait(ar::await_with_error());
        // NOTE: when the error is not cancel
        if (ec != asio::error::operation_aborted) {
          Logger::warn(fmt::format("Error while waiting timer: {}", ec.message()));
          break;
        }
        continue;
      }

      auto msg = std::move(write_message_queue_.front());

      // send 2 bufer in one go
      std::vector<asio::const_buffer> buffers{};
      buffers.emplace_back(asio::buffer(msg.header));
      buffers.emplace_back(asio::buffer(msg.body));
      auto expected_bytes = msg.size();
      auto [ec, n] = co_await asio::async_write(socket_, buffers, ar::await_with_error());
      if (ec) {
        if (is_connection_lost(ec))
          break;
        Logger::warn(fmt::format("Connection-{} error on sending message: {}", id_, ec.message()));
        continue;
      }
      if (n != expected_bytes) {
        Logger::warn(fmt::format("Connection-{} is sending message with different size: {}", id_, n));
        continue;
      }

      Logger::info(fmt::format("success sent 1 message to connection-{}!", id_));
      write_message_queue_.pop();
    }
    Logger::trace(fmt::format("Connection-{} no longer sending message!", id_));
    // if (m_connection_handler)
    // m_connection_handler->on_connection_closed(*this); // TODO: Change this, when the connection removed from connection_handler, the class itself could be still used on some async actions
  }
} // ar
