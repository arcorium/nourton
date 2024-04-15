//
// Created by mizzh on 4/15/2024.
//

#include "server.h"

#include <asio/bind_executor.hpp>
#include <fmt/format.h>

#include "user.h"
#include "logger.h"

namespace ar
{
  Server::Server(asio::io_context& context, const asio::ip::address& address, asio::ip::port_type port) :
    m_context{context}, m_strand{context.get_executor()},
    m_acceptor{m_context, asio::ip::tcp::endpoint{address, port}}
    , m_user_id_generator{1}
  {
  }

  Server::Server(asio::io_context& context, const asio::ip::tcp::endpoint& endpoint) :
    m_context{context}, m_strand{context.get_executor()}, m_acceptor{m_context, endpoint}
    , m_user_id_generator{1}
  {
  }

  void Server::start()
  {
    asio::post(m_context, [this] {
      Logger::trace("server start accept connection");
      m_acceptor.async_accept(
        asio::bind_executor(m_strand, std::bind(&Server::connection_handler, this, std::placeholders::_1,
                                                std::placeholders::_2)));
    });
  }

  void Server::on_message_in(Connection& conn, const Message& msg) noexcept
  {
    auto header = msg.get_header();
    switch (header->message_type)
    {
    case Message::Type::Login:
      {
        auto payload = msg.body_as<LoginPayload>();
        User* user = login_message_handler(conn, payload);
        conn.user(user);
        FeedbackPayload resp{PayloadId::Login, user != nullptr, std::nullopt};
        conn.write(resp.serialize()); // TODO: use asio::post instead?
      }
    case Message::Type::Register:
      {
        auto payload = msg.body_as<RegisterPayload>();
        bool result = register_message_handler(std::move(payload));
        FeedbackPayload resp{PayloadId::Register, result, std::nullopt};
        conn.write(resp.serialize());
      }
    case Message::Type::GetUserOnline:
      break;
    case Message::Type::GetUserDetails:
      break;
    case Message::Type::SendFile:
      {
        auto payload = msg.body_as<SendFilePayload>();
        auto& connections = m_user_connections[header->opponent_id];

        auto serialized = payload.serialize(conn.user()->id);

        // Send to all clients connected to specific user
        for (auto& connId : connections)
        {
          const auto con = std::ranges::find_if(m_connections, [=](const std::unique_ptr<Connection>& conn) {
            return conn->id() == connId;
          });

          if (con == m_connections.end())
            continue;

          asio::post(conn.socket().get_executor(), [&] { con->get()->write(serialized); });
        }
      }
      break;
    }
  }

  void Server::on_message_out(Connection& conn, std::span<const u8> bytes) noexcept
  {
  }

  void Server::on_connection_closed(Connection& conn) noexcept
  {
    if (conn.is_authenticated())
    {
      auto user_id = conn.user()->id;
      auto& connections = m_user_connections[user_id];
      std::erase(connections, conn.id());

      // remove from map when the user no longer has connection
      if (connections.empty())
      {
        m_user_connections.erase(user_id);
      }
    }

    std::erase_if(m_connections, [rconn = &conn](const std::unique_ptr<Connection>& conn) {
      return rconn->id() == conn->id();
    });
  }

  void Server::connection_handler(const asio::error_code& ec, asio::ip::tcp::socket&& socket) noexcept
  {
    if (ec)
    {
      Logger::warn(fmt::format("error on accept connection: {}", ec.message()));
      start();
      return;
    }

    auto conn = Connection::make_unique(std::forward<asio::ip::tcp::socket>(socket), *this, *this);
    Logger::info(fmt::format("new connection with id: {}", conn->id()));
    conn->start();
    m_connections.push_back(std::move(conn));
  }

  User* Server::login_message_handler(Connection& conn, const LoginPayload& payload) noexcept
  {
    const auto user = std::find_if(m_users.begin(), m_users.end(),
                                   [username = std::string_view{payload.username}](
                                   const std::unique_ptr<User>& usr) {
                                     return username == usr->name;
                                   });

    if (user == m_users.end())
      return nullptr;

    if (user->get()->password != payload.password)
      return nullptr;

    auto& connections = m_user_connections[user->get()->id];
    connections.emplace_back(conn.id());
    return user->get();
  }

  bool Server::register_message_handler(RegisterPayload&& payload) noexcept
  {
    const auto user = std::ranges::find_if(m_users,
                                           [username = std::string_view{payload.username}](
                                           const std::unique_ptr<User>& usr) {
                                             return username == usr->name;
                                           });

    // user with username already exists
    if (user != m_users.end())
      return false;
    m_users.emplace_back(std::make_unique<User>(m_user_id_generator.gen(), std::move(payload.username),
                                                std::move(payload.password), std::vector<u8>{}));
    return true;
  }
}
