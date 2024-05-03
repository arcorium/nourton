#pragma once

#include <asio/awaitable.hpp>
#include <asio/execution_context.hpp>
#include <asio/ip/tcp.hpp>
#include <string_view>

#include "connection.h"
#include "util/asio.h"
#include "util/types.h"

namespace ar
{
  class IEventHandler;

  class Client
  {
  public:
    Client(asio::any_io_executor executor, asio::ip::address address, u16 port,
           IEventHandler* event_handler) noexcept;
    Client(asio::any_io_executor executor, asio::ip::tcp::endpoint endpoint,
           IEventHandler* event_handler) noexcept;
    ~Client() noexcept;

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    Client(Client&& other) noexcept;
    Client& operator=(Client&&) = delete;

    asio::awaitable<bool> start() noexcept;

    void disconnect() noexcept;

    bool is_connected() const noexcept;

    template <typename Self>
    auto&& connection(this Self&& self) noexcept;

    template <typename Self>
    auto&& executor(this Self&& self) noexcept;

  private:
    asio::awaitable<void> reader() noexcept;

    void message_handler(const Message& msg) noexcept;

  private:
    IEventHandler* event_handler_;
    asio::any_io_executor executor_;
    asio::ip::tcp::endpoint endpoint_;

    Connection connection_;
  };

  template <typename Self>
  auto&& Client::connection(this Self&& self) noexcept
  {
    return std::forward<Self>(self).connection_;
  }

  template <typename Self>
  auto&& Client::executor(this Self&& self) noexcept
  {
    return std::forward<Self>(self).executor_;
  }
}  // namespace ar
