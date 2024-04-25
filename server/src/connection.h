//
// Created by mizzh on 4/19/2024.
//
#pragma once
#include <atomic>

#include <asio.hpp>
#include <queue>

#include "message/payload.h"
#include "util/literal.h"

namespace ar
{
  struct User;
  class IConnectionHandler;
  class IMessageHandler;

  class Connection : public std::enable_shared_from_this<Connection>
  {
  public:
    using id_type = u16;
    Connection(asio::ip::tcp::socket&& socket, IMessageHandler* message_handler,
               IConnectionHandler* connection_handler) noexcept;

    template <typename T> requires std::derived_from<T, IMessageHandler> && std::derived_from<T, IConnectionHandler>
    Connection(asio::ip::tcp::socket&& socket, T* handler) noexcept;

    ~Connection() noexcept;

    Connection(const Connection& other) = delete;

    Connection(Connection&& other) noexcept;

    Connection& operator=(const Connection& other) = delete;

    Connection& operator=(Connection&& other) noexcept;

    static std::shared_ptr<Connection> make_shared(asio::ip::tcp::socket&& socket, IMessageHandler* message_handler,
                                                   IConnectionHandler* connection_handler) noexcept;

    template <typename T> requires std::derived_from<T, IMessageHandler> && std::derived_from<T, IConnectionHandler>
    static std::shared_ptr<Connection> make_shared(asio::ip::tcp::socket&& socket, T* handler) noexcept;

    // Start reading and writing handler
    void start() noexcept;

    void write(Message&& msg) noexcept;

    bool is_open() const noexcept;

    [[nodiscard]]
    bool is_authenticated() const noexcept;

    template <typename Self>
    auto&& socket(this Self&& self) noexcept;

    template <typename Self>
    auto&& id(this Self&& self) noexcept;

    template <typename Self>
    auto&& user(this Self&& self) noexcept;

    void user(User* user) noexcept;

  private:
    void close() noexcept;

    asio::awaitable<void> reader() noexcept;
    asio::awaitable<void> writer() noexcept;

  private:
    inline static std::atomic<id_type> s_current_id = 1_u16;

    // TODO: Use concept template instead of interface
    IMessageHandler* message_handler_;
    IConnectionHandler* connection_handler_;

    id_type id_;
    User* user_;

    std::atomic_bool is_closing_;

    asio::steady_timer write_timer_;
    std::queue<Message> write_message_queue_; // WARN: need mutex?
    asio::ip::tcp::socket socket_;
  };

  template <typename T> requires std::derived_from<T, IMessageHandler> && std::derived_from<T, IConnectionHandler>
  Connection::Connection(asio::ip::tcp::socket&& socket, T* handler) noexcept
    : Connection{std::forward<decltype(socket)>(socket), handler, handler}
  {
  }

  template <typename T> requires std::derived_from<T, IMessageHandler> && std::derived_from<T, IConnectionHandler>
  std::shared_ptr<Connection> Connection::make_shared(asio::ip::tcp::socket&& socket, T* handler) noexcept
  {
    return std::make_shared<Connection>(std::forward<decltype(socket)>(socket), handler, handler);
  }

  template <typename Self>
  auto&& Connection::socket(this Self&& self) noexcept
  {
    return std::forward<Self>(self).socket_;
  }

  template <typename Self>
  auto&& Connection::id(this Self&& self) noexcept
  {
    return std::forward<Self>(self).id_;
  }

  template <typename Self>
  auto&& Connection::user(this Self&& self) noexcept
  {
    return std::forward<Self>(self).user_;
  }
} // ar

