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
    using context_type = Connection::context_type;

    Client(context_type& context, asio::ip::address address, u16 port) noexcept;
    Client(context_type& context, asio::ip::tcp::endpoint endpoint) noexcept;
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

  private:
    asio::awaitable<void> reader() noexcept;

    void message_handler(const Message& msg) noexcept;

  private:
    context_type& m_context;
    asio::ip::tcp::endpoint m_endpoint;

    Connection m_connection;
  };

  template <typename Self>
  auto&& Client::connection(this Self&& self) noexcept
  {
    return std::forward<Self>(self).m_connection;
  }
}
