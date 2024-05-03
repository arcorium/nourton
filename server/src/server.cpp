#include "server.h"

#include <fmt/format.h>
#include <util/asio.h>

#include <asio/bind_executor.hpp>
#include <asio/placeholders.hpp>
#include <magic_enum.hpp>

#include "core.h"
#include "logger.h"
#include "message/payload.h"
#include "user.h"

namespace ar
{
  namespace me = magic_enum;

  Server::Server(asio::any_io_executor executor, const asio::ip::address& address,
                 asio::ip::port_type port)
      : Server(std::move(executor), asio::ip::tcp::endpoint{address, port})
  {
  }

  Server::Server(asio::any_io_executor executor, const asio::ip::tcp::endpoint& endpoint)
      : executor_{std::move(executor)},
        strand_{executor_},
        acceptor_{executor_, endpoint},
        user_id_generator_{1}
  {
  }

  void Server::start() noexcept
  {
    Logger::info(fmt::format("server accepting connection from {}: {}",
                             acceptor_.local_endpoint().address().to_string(),
                             acceptor_.local_endpoint().port()));

    asio::co_spawn(strand_, [this] { return connection_acceptor(); }, asio::detached);

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
            .id = PayloadId::Login,
            .response = false,
            .message = std::string{AUTHENTICATED_MESSAGE},
        };
        conn.write(resp_payload.serialize());
        return;
      }

      auto payload = msg.body_as<LoginPayload>();
      User* user = login_message_handler(conn, payload);

      if (user)
      {
        // send signal UserLogin payload
        UserLoginPayload login_payload{user->id, user->name};
        broadcast(login_payload.serialize(), user->id);
      }

      // DEBUG
      if constexpr (AR_DEBUG)
      {
        if (!user)
          Logger::warn(fmt::format("connection-{} failed to authenticate with username: {}",
                                   conn.id(), payload.username));
        else
          Logger::info(fmt::format("connection-{} logged in successfully with username: {}",
                                   conn.id(), payload.username));
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
            .id = PayloadId::Register,
            .response = false,
            .message = std::string{AUTHENTICATED_MESSAGE},
        };
        conn.write(resp_payload.serialize());
        return;
      }

      auto payload = msg.body_as<RegisterPayload>();
      bool result = register_message_handler(std::move(payload));
      if constexpr (AR_DEBUG)
      {
        if (!result)
          Logger::info(fmt::format("connection-{} failed on registering with username: {}",
                                   conn.id(), payload.username));
        else
          Logger::info(fmt::format("connection-{} registered successfully with username: {}",
                                   conn.id(), payload.username));
      }
      FeedbackPayload resp{PayloadId::Register, result, std::nullopt};
      conn.write(resp.serialize());
      break;
    }
    case Message::Type::StorePublicKey: {
      // prevent authenticated connection
      if (!conn.is_authenticated())
      {
        FeedbackPayload resp_payload{
            .id = PayloadId::StorePublicKey,
            .response = false,
            .message = std::string{UNAUTHENTICATED_MESSAGE},
        };
        conn.write(resp_payload.serialize());
        break;
      }

      auto payload = msg.body_as<StorePublicKeyPayload>();
      auto it = std::ranges::find_if(
          users_, [&](const std::unique_ptr<User>& user) { return user->id == conn.user()->id; });

      FeedbackPayload resp_payload;
      if (it == users_.end())
      {
        // user not found
        Logger::error(fmt::format(
            "connection-{} trying to store public key to user that doesn't exists", conn.id()));
        resp_payload = FeedbackPayload{
            .id = PayloadId::StorePublicKey,
            .response = false,
        };
      }
      else
      {
        // save public key
        it->get()->public_key = std::move(payload.public_key);

        resp_payload = FeedbackPayload{
            .id = PayloadId::StorePublicKey,
            .response = true,
        };
      }

      conn.write(resp_payload.serialize());
      break;
    }
    case Message::Type::GetUserOnline: {
      // prevent authenticated connection
      if (!conn.is_authenticated())
      {
        FeedbackPayload resp_payload{
            .id = PayloadId::GetUserOnline,
            .response = false,
            .message = std::string{UNAUTHENTICATED_MESSAGE},
        };
        conn.write(resp_payload.serialize());
        break;
      }

      // filter online users
      std::vector<UserResponse> users;
      // users.reserve(m_users.size() - 1);
      for (const auto& con : connections_)
      {
        if (!con->user() || con->user()->id == conn.user()->id)
          continue;
        users.emplace_back(con->user()->id, con->user()->name);  // make sure password is not sent
      }

      UserOnlinePayload payload{.users = std::move(users)};

      conn.write(payload.serialize());
      break;
    }
    case Message::Type::GetUserDetails: {
      // prevent authenticated connection
      if (!conn.is_authenticated())
      {
        FeedbackPayload resp_payload{
            .id = PayloadId::GetUserDetails,
            .response = false,
            .message = std::string{UNAUTHENTICATED_MESSAGE},
        };
        conn.write(resp_payload.serialize());
        break;
      }

      auto payload = msg.body_as<GetUserDetailsPayload>();
      // get the user
      auto it = std::ranges::find_if(
          users_, [&](const std::unique_ptr<User>& user) { return user->id == payload.id; });

      UserDetailPayload resp_payload;
      if (it == users_.end())
      {
        Logger::warn(fmt::format("connection-{} trying to get user-{} details that doesn't exists",
                                 conn.id(), payload.id));
        // bad payload
        resp_payload = UserDetailPayload{
            .id = 0,
            .username = "",
        };
      }
      else
      {
        resp_payload = UserDetailPayload{.id = it->get()->id,
                                         .username = it->get()->name,
                                         .public_key = it->get()->public_key};
      }

      conn.write(resp_payload.serialize());

      break;
    }
    case Message::Type::SendFile: {
      // check if authenticated
      if (!conn.is_authenticated())
      {
        FeedbackPayload resp_payload{
            .id = PayloadId::SendFile,
            .response = false,
            .message = std::string{UNAUTHENTICATED_MESSAGE},
        };
        conn.write(resp_payload.serialize());
        return;
      }

      auto payload = msg.body_as<SendFilePayload>();
      // check if user is online
      if (!user_connections_.contains(header->opponent_id))
      {
        FeedbackPayload resp_payload{.id = PayloadId::SendFile,
                                     .response = false,
                                     .message = "user doesn't online"};
        conn.write(resp_payload.serialize());
        return;
      }

      auto& connections = user_connections_[header->opponent_id];

      if constexpr (AR_DEBUG)
      {
        // get the user opponent
        auto user = std::ranges::find_if(users_, [&header](const std::unique_ptr<User>& usr) {
          return usr->id == header->opponent_id;
        });

        Logger::info(
            fmt::format("user-{} sending files to user-{}", conn.user()->name, user->get()->name));
      }

      // Send to all clients connected to specific user
      for (auto& connId : connections)
      {
        // change opponent id into sender id
        auto serialized = payload.serialize(conn.user()->id);

        const auto con = std::ranges::find_if(
            connections_,
            [=](const std::weak_ptr<Connection> conn) { return conn.lock()->id() == connId; });

        if (con == connections_.end())
        {
          Logger::warn(
              fmt::format("user connections has id that not belongs to any client: {}", connId));
          continue;
        }

        con->get()->write(std::move(serialized));
      }

      FeedbackPayload resp_payload{
          .id = PayloadId::SendFile,
          .response = true,
      };
      conn.write(resp_payload.serialize());
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
      // remove from user connections map
      auto user_id = conn.user()->id;
      auto& connections = user_connections_[user_id];
      std::erase(connections, conn.id());

      // remove from map when the user no longer has connection
      if (connections.empty())
      {
        user_connections_.erase(user_id);
        // send signal UserLogout payload except for current user
        UserLogoutPayload payload{user_id};
        broadcast(payload.serialize(), user_id);
      }
    }

    // delete client connection
    std::erase_if(connections_, [rconn = &conn](const std::weak_ptr<Connection>& conn) {
      return rconn->id() == conn.lock()->id();
    });
  }

  asio::awaitable<void> Server::connection_acceptor() noexcept
  {
    while (true)
    {
      auto [ec, socket] = co_await acceptor_.async_accept(ar::await_with_error());
      if (ec)
      {
        Logger::warn(fmt::format("error on accepting connection: {}", ec.message()));
        break;
      }

      auto conn = Connection::make_shared(std::forward<asio::ip::tcp::socket>(socket), this);
      Logger::info(fmt::format("new connection with id: {}", conn->id()));
      conn->start();
      connections_.push_back(std::move(conn));
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

    auto conn = Connection::make_shared(std::forward<asio::ip::tcp::socket>(socket), this, this);
    Logger::info(fmt::format("new connection with id: {}", conn->id()));
    conn->start();
    connections_.push_back(std::move(conn));
    start();
  }

  User* Server::login_message_handler(Connection& conn, const LoginPayload& payload) noexcept
  {
    const auto user = std::ranges::find_if(
        users_, [username = std::string_view{payload.username}](const std::unique_ptr<User>& usr) {
          return username == usr->name;
        });

    if (user == users_.end())
      return nullptr;

    if (user->get()->password != payload.password)
      return nullptr;

    auto& connections = user_connections_[user->get()->id];
    connections.emplace_back(conn.id());
    return user->get();
  }

  bool Server::register_message_handler(RegisterPayload&& payload) noexcept
  {
    const auto user = std::ranges::find_if(
        users_, [username = std::string_view{payload.username}](const std::unique_ptr<User>& usr) {
          return username == usr->name;
        });

    // user with username already exists
    if (user != users_.end())
      return false;
    users_.emplace_back(std::make_unique<User>(user_id_generator_.gen(),
                                                std::move(payload.username),
                                                std::move(payload.password), std::vector<u8>{}));
    return true;
  }

  void Server::broadcast(Message&& message, User::id_type except) noexcept
  {
    Logger::trace(fmt::format("Broadcasting message except for client {}", except));

    for (const auto& conn : connections_)
    {
      // ignore unauthenticated connection and excluded user id
      if (!conn->user() || conn->user()->id == except)
        continue;

      // need to copy each message, because it will moved fo each writing
      auto msg_copy = message;
      conn->write(std::move(msg_copy));
    }
  }
}  // namespace ar
