//
// Created by arcorium on 4/16/2024.
//

#include "client.h"

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/thread_pool.hpp>
#include <utility>

#include <fmt/format.h>

#include "logger.h"


namespace ar
{
  Client::Client(asio::any_io_executor executor, asio::ip::address address,
                 u16 port) noexcept
    : Client{std::move(executor), asio::ip::tcp::endpoint{address, port}}
  {
  }

  Client::Client(asio::any_io_executor executor, asio::ip::tcp::endpoint endpoint) noexcept
    : m_executor{std::move(executor)}, m_endpoint{std::move(endpoint)},
      m_connection{m_executor}
  {
  }

  Client::~Client() noexcept
  {
  }

  Client::Client(Client&& other) noexcept
    : m_executor{std::move(other.m_executor)}, m_endpoint{std::move(other.m_endpoint)},
      m_connection{std::move(other.m_connection)}
  {
  }

  asio::awaitable<bool> Client::start() noexcept
  {
    Logger::trace("Application trying to connect...");
    auto ec = co_await m_connection.connect(m_endpoint);
    if (ec)
    {
      Logger::critical(
        fmt::format("failed to connect into endpoint : {}", ec.message()));
      co_return false;
    }

    Logger::info("Application connected to endpoint");

    // reader coroutine
    asio::co_spawn(m_executor, [this] { return reader(); }, asio::detached);
    // writer coroutine
    m_connection.start();

    co_return true;
  }

  void Client::disconnect() noexcept { m_connection.close(); }

  bool Client::is_connected() const noexcept { return m_connection.is_open(); }

  asio::awaitable<void> Client::reader() noexcept
  {
    Logger::trace("Client start reading data from remote");
    while (m_connection.socket().is_open())
    {
      auto msg = co_await m_connection.read();
      if (!msg.has_value())
      {
        Logger::warn(fmt::format("Failed to read data: {}", msg.error().message()));
        break;
      }
      Logger::info("Client got message from remote");
      message_handler(msg.value());
    }

    asio::post(m_executor, [this] {
      disconnect();
    });
  }

  void Client::message_handler(const Message& msg) noexcept
  {
  }
} // namespace ar
