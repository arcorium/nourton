//
// Created by mizzh on 4/15/2024.
//

#include "connection.h"

#include <fmt/format.h>

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/placeholders.hpp>
#include <asio/read.hpp>
#include <asio/redirect_error.hpp>
#include <asio/thread_pool.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/write.hpp>
#include <asio/awaitable.hpp>

#include "core.h"
#include "logger.h"
#include "message/payload.h"
#include "util/asio.h"
#include "util/make.h"

namespace ar
{
  Connection::Connection(asio::any_io_executor executor) noexcept
      : is_closing_{true},
        socket_{std::move(executor)},
        write_timer_{socket_.get_executor(), std::chrono::steady_clock::time_point::max()}
  {
  }

  Connection::Connection(Connection&& other) noexcept
      : is_closing_{other.is_closing_.exchange(true)},
        write_message_queue_{std::move(other.write_message_queue_)},
        socket_{std::move(other.socket())},
        write_timer_{std::move(other.write_timer_)},
        user_{std::move(other.user_)}
  {
  }

  Connection& Connection::operator=(Connection&& other) noexcept
  {
    if (this == &other)
      return *this;

    write_timer_ = std::move(other.write_timer_);
    write_message_queue_ = std::move(other.write_message_queue_);
    socket_ = std::move(other.socket_);
    user_ = std::move(other.user_);
    is_closing_ = other.is_closing_.exchange(!other.is_closing_.load());
    return *this;
  }

  Connection::~Connection() noexcept = default;

  asio::awaitable<asio::error_code> Connection::connect(
      const asio::ip::tcp::endpoint& endpoint) noexcept
  {
    auto [ec] = co_await socket_.async_connect(endpoint, ar::await_with_error());
    co_return ec;
  }

  void Connection::start() noexcept
  {
    Logger::info("Connection started!");
    is_closing_.store(false);
    asio::co_spawn(socket_.get_executor(), [this] { return write_handler(); }, asio::detached);
  }

  void Connection::close() noexcept
  {
    if (is_closing_.load())
      return;
    is_closing_.store(true);
    write_timer_.cancel();
    socket_.cancel();
    socket_.close();
    Logger::info("connection closed!");
  }

  asio::awaitable<std::expected<Message, asio::error_code>> Connection::read() noexcept
  {
    Logger::trace("Connection waiting new message from remote...");
    Message message{};
    {
      auto [ec, n] = co_await asio::async_read(socket_, asio::buffer(message.header),
                                               asio::transfer_exactly(Message::header_size),
                                               ar::await_with_error());
      if (ec || n != Message::header_size)
        co_return std::unexpected(ec);
    }

    auto header = message.as_header();
    message.body.resize(
        header
            ->body_size);  // NOTE: need to resize instead of reserve because of using asio::buffer
    auto [ec, n] = co_await asio::async_read(socket_, asio::buffer(message.body),
                                             asio::transfer_exactly(header->body_size),
                                             ar::await_with_error());
    if (ec || n != header->body_size)
      co_return std::unexpected(ec);

    Logger::info(fmt::format("Connection got data {} bytes", message.size()));
    co_return ar::make_expected<Message, asio::error_code>(std::move(message));
  }

  bool Connection::is_open() const noexcept
  {
    return socket_.is_open() && !is_closing_.load();
  }

  void Connection::write(Message&& msg) noexcept
  {
    if (!is_open())
    {
      Logger::warn("Could not send data due to remote connection already closed");
      return;
    }

    Logger::trace(fmt::format("Connection send data {} bytes", msg.size()));
    write_message_queue_.push(std::forward<Message>(msg));
    asio::error_code ec;
    write_timer_.cancel_one(ec);
    if constexpr (AR_DEBUG)
    {
      if (ec)
        Logger::warn(fmt::format("failed to cancel timer: {}", ec.message()));
    }
  }

  asio::awaitable<void> Connection::write_handler() noexcept
  {
    while (is_open())
    {
      if (write_message_queue_.empty())
      {
        Logger::trace("connection waiting for new message to write");
        auto [ec] = co_await write_timer_.async_wait(ar::await_with_error());
        // WARN: in case the timer is expired
        // if (!ec)
        // m_send_timer.expires_from_now(std::chrono::duration<std::chrono::days>::max());
        // cancelled
        if (ec != asio::error::operation_aborted)
        {
          Logger::trace(fmt::format("Error while waiting timer: {}", ec.message()));
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
      if (ec)
      {
        if (ar::is_connection_lost(ec))
          break;
        Logger::warn(fmt::format("failed to send message: {}", ec.message()));
        continue;
      }
      if (n != expected_bytes)
      {
        Logger::warn(fmt::format("message has different size: {}", n));
        continue;
      }

      Logger::info("success sent 1 message!");
      write_message_queue_.pop();
    }
    Logger::trace("connection no longer run write handler");
    // close(); TODO: Should be called from outside class
  }
}  // namespace ar
