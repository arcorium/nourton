#pragma once
#include <string_view>

#include <asio/awaitable.hpp>
#include <asio/execution_context.hpp>
#include <asio/ip/tcp.hpp>

#include "connection.h"

#include "util/types.h"
#include "util/asio.h"

namespace ar
{
  class Client
  {
  public:
    Client(asio::any_io_executor executor, asio::ip::address address, u16 port) noexcept;
    Client(asio::any_io_executor executor, asio::ip::tcp::endpoint endpoint) noexcept;
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
    asio::any_io_executor m_executor;
    asio::ip::tcp::endpoint m_endpoint;

    Connection m_connection;
  };

  template <typename Self>
  auto&& Client::connection(this Self&& self) noexcept
  {
    return std::forward<Self>(self).m_connection;
  }

  template <typename Self>
  auto&& Client::executor(this Self&& self) noexcept
  {
    return std::forward<Self>(self).m_executor;
  }
}
