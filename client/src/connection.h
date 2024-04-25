#pragma once
#include <queue>

#include <asio/awaitable.hpp>

#include "message/payload.h"
#include "util/literal.h"
#include "util/types.h"

#include <asio/ip/tcp.hpp>

namespace ar
{
  struct User;

  class Connection
  {
  public:
    using id_type = u16;

    explicit Connection(asio::any_io_executor executor) noexcept;

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    Connection(Connection&& other) noexcept;
    Connection& operator=(Connection&& other) noexcept;

    ~Connection() noexcept;

    asio::awaitable<asio::error_code> connect(const asio::ip::tcp::endpoint& endpoint) noexcept;

    void start() noexcept;

    // it is okay to call it multiple times
    void close() noexcept;

    asio::awaitable<std::expected<Message, asio::error_code>> read() noexcept;

    bool is_open() const noexcept;

    void write(Message&& msg) noexcept;

    template <typename Self>
    auto&& socket(this Self&& self) noexcept;

    template <typename Self>
    auto&& user(this Self&& self) noexcept;

    void user(User* user) noexcept;

  private:
    asio::awaitable<void> write_handler() noexcept;

  private:
    std::atomic_bool is_closing_;

    // Message m_input_message;
    std::queue<Message> write_message_queue_;
    asio::ip::tcp::socket socket_;

    // correspond authenticated user, it will be null when the connection is not authenticated yet
    asio::steady_timer write_timer_;
    std::unique_ptr<User> user_;
  };

  template <typename Self>
  auto&& Connection::socket(this Self&& self) noexcept
  {
    return std::forward<Self>(self).socket_;
  }

  template <typename Self>
  auto&& Connection::user(this Self&& self) noexcept
  {
    return std::forward<Self>(self).user_;
  }
}
