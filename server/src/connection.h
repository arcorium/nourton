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

    Connection(Connection&& other) noexcept
      : m_message_handler(other.m_message_handler),
        m_connection_handler(other.m_connection_handler),
        m_id(other.m_id),
        m_user(other.m_user),
        m_is_closing(other.m_is_closing.exchange(true)),
        m_write_timer(std::move(other.m_write_timer)),
        m_write_buffers(std::move(other.m_write_buffers)),
        m_socket(std::move(other.m_socket))
    {
      other.m_id = std::numeric_limits<id_type>::max();
      other.m_user = nullptr;
      other.m_connection_handler = nullptr;
      other.m_message_handler = nullptr;
    }

    Connection& operator=(const Connection& other) = delete;

    Connection& operator=(Connection&& other) noexcept
    {
      if (this == &other)
        return *this;
      m_message_handler = other.m_message_handler;
      m_connection_handler = other.m_connection_handler;
      m_id = other.m_id;
      m_user = other.m_user;
      m_is_closing = other.m_is_closing.exchange(true);
      m_write_timer = std::move(other.m_write_timer);
      m_write_buffers = std::move(other.m_write_buffers);
      m_socket = std::move(other.m_socket);


      other.m_id = std::numeric_limits<id_type>::max();
      other.m_user = nullptr;
      other.m_connection_handler = nullptr;
      other.m_message_handler = nullptr;
      return *this;
    }

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
    IMessageHandler* m_message_handler;
    IConnectionHandler* m_connection_handler;

    id_type m_id;
    User* m_user;

    std::atomic_bool m_is_closing;

    asio::steady_timer m_write_timer;
    std::queue<Message> m_write_buffers;
    asio::ip::tcp::socket m_socket;
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
    return std::forward<Self>(self).m_socket;
  }

  template <typename Self>
  auto&& Connection::id(this Self&& self) noexcept
  {
    return std::forward<Self>(self).m_id;
  }

  template <typename Self>
  auto&& Connection::user(this Self&& self) noexcept
  {
    return std::forward<Self>(self).m_user;
  }
} // ar

