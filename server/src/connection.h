#pragma once
#include <queue>

#include "message/payload.h"
#include "util/literal.h"
#include "util/types.h"

#include <asio/ip/tcp.hpp>

#include "handler.h"
// #include "user.h"

namespace ar
{
  class Connection
  {
  public:
    using id_type = u16;

    explicit Connection(asio::ip::tcp::socket &&socket, IMessageHandler &message_handler,
                        IConnectionHandler &connection_handler) noexcept;

    ~Connection() noexcept;

    static std::unique_ptr<Connection> make_unique(asio::ip::tcp::socket &&socket,
                                                   IMessageHandler &message_handler,
                                                   IConnectionHandler &connection_handler) noexcept;

    void start() noexcept;
    void read_header() noexcept;
    void read_body() noexcept;
    void write(const Message &msg) noexcept;

    bool is_authenticated() const noexcept;

    template <typename Self>
    auto &&socket(this Self &&self) noexcept;

    template <typename Self>
    auto &&id(this Self &&self) noexcept;

    template <typename Self>
    auto &&user(this Self &&self) noexcept;

    void user(User *user) noexcept;

  private:
    void stop() noexcept;
    void close() noexcept;
    void read_header_handler(const asio::error_code &ec, usize n) noexcept;
    void read_body_handler(const asio::error_code &ec, usize n) noexcept;
    void write_handler(const asio::error_code &ec, usize n) noexcept;

  private:
    inline static std::atomic<id_type> s_current_id = 1_u16;
    // 0 is reserved for server
    IMessageHandler &m_message_handler;
    IConnectionHandler &m_connection_handler;

    id_type m_id;
    std::atomic_bool m_is_writing;
    std::atomic_bool m_is_closing;

    Message m_input_message;
    std::queue<std::vector<u8>> m_write_buffer;
    asio::ip::tcp::socket m_socket;

    // correspond authenticated user, it will be null when the connection is not authenticated yet
    User *m_user;
  };

  template <typename Self>
  auto &&Connection::socket(this Self &&self) noexcept
  {
    return std::forward<Self>(self).m_socket;
  }

  template <typename Self>
  auto &&Connection::id(this Self &&self) noexcept
  {
    return std::forward<Self>(self).m_id;
  }

  template <typename Self>
  auto &&Connection::user(this Self &&self) noexcept
  {
    return std::forward<Self>(self).m_user;
  }

}
