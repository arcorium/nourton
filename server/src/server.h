#pragma once
#include <asio/ip/tcp.hpp>
#include <asio/strand.hpp>
#include <map>
#include <memory>
#include <span>
#include <vector>

#include "connection.h"
#include "generator.h"
#include "handler.h"

namespace ar
{
  struct LoginPayload;
  struct RegisterPayload;
  struct User;

  class Server : public IMessageHandler, public IConnectionHandler
  {
  public:
    Server(asio::any_io_executor executor, const asio::ip::address& address,
           asio::ip::port_type port);
    Server(asio::any_io_executor executor, const asio::ip::tcp::endpoint& endpoint);

    void start() noexcept;

    void on_message_in(Connection& conn, const Message& msg) noexcept override;
    void on_message_out(Connection& conn, std::span<const u8> bytes) noexcept override;
    void on_connection_closed(Connection& conn) noexcept override;

  private:
    asio::awaitable<void> connection_acceptor() noexcept;

    User* login_message_handler(Connection& conn, const LoginPayload& payload) noexcept;
    bool register_message_handler(RegisterPayload&& payload) noexcept;

    void broadcast(Message&& message, User::id_type except = 0) noexcept;

  private:
    asio::any_io_executor executor_;
    asio::strand<asio::any_io_executor> strand_;
    asio::ip::tcp::acceptor acceptor_;

    std::vector<std::shared_ptr<Connection>> connections_;  // client database
    // map user-connections, instead of checking user inside each connection
    std::map<u16, std::vector<u16>> user_connections_;
    std::vector<std::unique_ptr<User>> users_;  // user database
    IdGenerator<u16> user_id_generator_;
  };
}  // namespace ar
