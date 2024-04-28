#include "server.h"

#include <asio/bind_executor.hpp>
#include <fmt/format.h>

#include <magic_enum.hpp>

#include <asio/placeholders.hpp>

#include <util/asio.h>

#include "logger.h"
#include "user.h"

#include "message/payload.h"

namespace ar
{
  namespace me = magic_enum;

  Server::Server(asio::any_io_executor executor, const asio::ip::address& address,
                 asio::ip::port_type port)
    : Server{std::move(executor), asio::ip::tcp::endpoint{address, port}} {}

  Server::Server(asio::any_io_executor executor,
                 const asio::ip::tcp::endpoint& endpoint)
    : m_executor{std::move(executor)}, m_strand{m_executor},
      m_acceptor{m_executor, endpoint}, m_user_id_generator{1} {}

  void Server::start() noexcept
  {
    Logger::trace(fmt::format("server accepting connection from {}: {}",
                              m_acceptor.local_endpoint().address().to_string(),
                              m_acceptor.local_endpoint().port()));

    asio::co_spawn(m_strand, [this] { return connection_accepter(); }, asio::detached);

    // m_acceptor.async_accept(asio::bind_executor(
    //   m_strand, std::bind(&Server::connection_handler, this,
    //                       asio::placeholders::error, std::placeholders::_2)));
  }

  void Server::on_message_in(Connection& conn, const Message& msg) noexcept
  {
    auto header = msg.as_header();
    Logger::trace(fmt::format("new message from connection-{}: {}", conn.id(),
                              me::enum_name(header->message_type)));
    switch (header->message_type)
    {
    case Message::Type::Login: {
      if (conn.is_authenticated())
      {
        FeedbackPayload resp_payload{
          .id = PayloadId::Authenticated,
          .response = false,
          .message = AUTHENTICATED_MESSAGE.data(),
        };
        conn.write(resp_payload.serialize());
        return;
      }

      auto payload = msg.body_as<LoginPayload>();
      User* user = login_message_handler(conn, payload);

      // DEBUG
      if constexpr (_DEBUG)
      {
        if (!user)
          Logger::warn(fmt::format(
            "connection-{} failed to authenticate with username: {}", conn.id(),
            payload.username));
        else
          Logger::info(fmt::format(
            "connection-{} logged in successfully with username: {}", conn.id(),
            payload.username));
      }

      conn.user(user);
      FeedbackPayload resp{PayloadId::Login, user != nullptr, std::nullopt};
      conn.write(resp.serialize());
      break;
    }
    case Message::Type::Register: {
      if (conn.is_authenticated())
      {
        FeedbackPayload resp_payload{
          .id = PayloadId::Authenticated,
          .response = false,
          .message = AUTHENTICATED_MESSAGE.data(),
        };
        conn.write(resp_payload.serialize());
        return;
      }

      auto payload = msg.body_as<RegisterPayload>();
      bool result = register_message_handler(std::move(payload));
      if constexpr (_DEBUG)
      {
        if (result)
          Logger::info(
            fmt::format("connection-{} failed on registering with username: {}",
                        conn.id(), payload.username));
        else
          Logger::info(fmt::format(
            "connection-{} registered successfully with username: {}",
            conn.id(), payload.username));
      }
      FeedbackPayload resp{PayloadId::Register, result, std::nullopt};
      conn.write(resp.serialize());
      break;
    }
    case Message::Type::GetUserOnline:
      break;
    case Message::Type::GetUserDetails:
      break;
    case Message::Type::SendFile: {
      // check if authenticated
      if (!conn.is_authenticated())
      {
        FeedbackPayload resp_payload{
          .id = PayloadId::Unauthenticated,
          .response = false,
          .message = UNAUTHENTICATED_MESSAGE.data(),
        };
        conn.write(resp_payload.serialize());
        return;
      }

      auto payload = msg.body_as<SendFilePayload>();
      // check if user is online
      if (!m_user_connections.contains(header->opponent_id))
      {
        FeedbackPayload resp_payload{
          .id = PayloadId::UserCheck,
          .response = false,
          .message = "user doesn't online"
        };
        conn.write(resp_payload.serialize());
        return;
      }

      auto& connections = m_user_connections[header->opponent_id];

      if constexpr (_DEBUG)
      {
        // get the user opponent
        auto user = std::ranges::find_if(
          m_users, [&header](const std::unique_ptr<User>& usr) {
            return usr->id == header->opponent_id;
          });

        Logger::info(fmt::format("user-{} sending files to user-{}",
                                 conn.user()->name, user->get()->name));
      }

      // Send to all clients connected to specific user
      for (auto& connId : connections)
      {
        auto serialized = payload.serialize(conn.user()->id);

        const auto con = std::ranges::find_if(
          m_connections, [=](const std::weak_ptr<Connection> conn) {
            return conn.lock()->id() == connId;
          });

        if (con == m_connections.end())
          continue;

        con->get()->write(std::move(serialized));
      }

      break;
    }
    }
  }

  void Server::on_message_out(Connection& conn, std::span<const u8> bytes) noexcept
  {
    Logger::trace(fmt::format("new message out to connection-{}", conn.id()));
  }

  void Server::on_connection_closed(Connection& conn) noexcept
  {
    Logger::trace(fmt::format("remove connection-{} from database", conn.id()));
    if (conn.is_authenticated())
    {
      auto user_id = conn.user()->id;
      auto& connections = m_user_connections[user_id];
      std::erase(connections, conn.id());

      // remove from map when the user no longer has connection
      if (connections.empty())
        m_user_connections.erase(user_id);
    }

    std::erase_if(m_connections,
                  [rconn = &conn](const std::weak_ptr<Connection>& conn) {
                    return rconn->id() == conn.lock()->id();
                  });
    auto count = conn.shared_from_this().use_count();
  }

  asio::awaitable<void> Server::connection_accepter() noexcept
  {
    while (true)
    {
      auto [ec, socket] = co_await m_acceptor.async_accept(ar::await_with_error());
      if (ec)
      {
        Logger::warn(fmt::format("error on accepting connection: {}", ec.message()));
        break;
      }

      auto conn = Connection::make_shared(
        std::forward<asio::ip::tcp::socket>(socket), this);
      Logger::info(fmt::format("new connection with id: {}", conn->id()));
      conn->start();
      m_connections.push_back(std::move(conn));
    }
  }

  void Server::connection_handler(const asio::error_code& ec,
                                  asio::ip::tcp::socket&& socket) noexcept
  {
    if (ec)
    {
      Logger::warn(fmt::format("error on accept connection: {}", ec.message()));
      start();
      return;
    }

    auto conn = Connection::make_shared(
      std::forward<asio::ip::tcp::socket>(socket), this, this);
    Logger::info(fmt::format("new connection with id: {}", conn->id()));
    conn->start();
    m_connections.push_back(std::move(conn));
    start();
  }

  User* Server::login_message_handler(Connection& conn,
                                      const LoginPayload& payload) noexcept
  {
    const auto user = std::ranges::find_if(
      m_users,
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
    const auto user = std::ranges::find_if(
      m_users,
      [username = std::string_view{payload.username}](
      const std::unique_ptr<User>& usr) {
        return username == usr->name;
      });

    // user with username already exists
    if (user != m_users.end())
      return false;
    m_users.emplace_back(std::make_unique<User>(
      m_user_id_generator.gen(), std::move(payload.username),
      std::move(payload.password), std::vector<u8>{}));
    return true;
  }
} // namespace ar
