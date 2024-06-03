#include "server.h"

#include <fmt/format.h>
#include <fmt/std.h>
#include <fmt/ranges.h>

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

    asio::co_spawn(strand_, [this] {
      return connection_acceptor();
    }, asio::detached);
  }

  void Server::on_message_in(Connection& conn, const Message& msg) noexcept
  {
    auto header = msg.as_header();
    Logger::trace(fmt::format("Connection-{} got {} message", conn.id(),
                              me::enum_name(header->message_type)));

    switch (header->message_type)
    {
    case Message::Type::Login: {
      login_message_handler(conn, msg);
      break;
    }
    case Message::Type::Register: {
      register_message_handler(conn, msg);
      break;
    }
    case Message::Type::GetUserOnline: {
      get_user_online_handler(conn, msg);
      break;
    }
    case Message::Type::GetUserDetails: {
      get_user_details_handler(conn, msg);
      break;
    }
    case Message::Type::GetServerDetail: {
      // Unencrypted
      get_server_details_handler(conn, msg);
      break;
    }
    case Message::Type::StorePublicKey: {
      // Encrypted using symmetric key
      store_public_key_handler(conn, msg);
      break;
    }
    case Message::Type::StoreSymmetricKey: {
      // Encrypted using server public key
      store_symmetric_key_handler(conn, msg);
      break;
    }
    case Message::Type::SendFile: {
      send_file_handler(conn, msg);
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
        broadcast_message<true>(payload, user_id);
      }
    }

    // delete client connection
    std::erase_if(connections_, [rconn = &conn](const std::weak_ptr<Connection>& conn) {
      return rconn->id() == conn.lock()->id();
    });
  }

  asio::awaitable<void> Server::connection_acceptor() noexcept
  {
    auto serialized_pk = ar::serialize(asymm_encryptor_.public_key());
    ServerDetailsPayload server_detail_payload{.id = User::SERVER_ID,
                                               .public_key = std::move(serialized_pk)};
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
      // Send server public key
      send_message(*conn, server_detail_payload);
      connections_.push_back(std::move(conn));
    }
  }

  void Server::send_message(Connection& conn, Message&& msg) noexcept
  {
    const auto header = msg.as_header();
    if (conn.user())
      Logger::info(fmt::format("Sending {} message to {}[{}]", me::enum_name(header->message_type),
                               conn.user()->name, conn.id()));
    else
      Logger::info(fmt::format("Sending {} message to {}", me::enum_name(header->message_type),
                               conn.id()));

    conn.write(std::forward<Message>(msg));
  }

  void Server::login_message_handler(Connection& conn, const Message& msg) noexcept
  {
    // All data encrypted using symmetric key
    auto header = msg.as_header();

    auto& symm_encryptor = conn.symmetric_encryptor();
    if (!expect_prologue<false, Message::EncryptionType::Symmetric, FeedbackId::Login>(
        symm_encryptor, conn, *header))
      return;

    auto payload = get_payload<LoginPayload>(symm_encryptor, msg);
    if (!payload)
    {
      Logger::error(fmt::format("Connection-{} sent malformed packet", conn.id()));
      send_feedback<false, FeedbackId::Login>(symm_encryptor, conn, payload.error());
      return;
    }

    auto aaa = payload.value();

    const auto user = std::ranges::find_if(
        users_, [username = std::string_view{payload->username}](const std::unique_ptr<User>& usr) {
          return username == usr->name;
        });

    if (user == users_.end())
    {
      Logger::warn(fmt::format("Connection-{} failed to authenticate with username: {}",
                               conn.id(), payload->username));

      send_feedback<false, FeedbackId::Login>(symm_encryptor, conn, USER_NOT_FOUND);
      return;
    }

    if (user->get()->password != payload->password)
    {
      Logger::warn(fmt::format("Connection-{} provide invalid password for username: {}",
                               conn.id(), payload->username));

      send_feedback<false, FeedbackId::Login>(symm_encryptor, conn, PASSWORD_INVALID);
      return;
    }

    // add connection into users
    auto& connections = user_connections_[user->get()->id];
    connections.emplace_back(conn.id());

    // send signal to other clients UserLogin payload
    UserLoginPayload login_payload{user->get()->id, user->get()->name};
    conn.user(user->get());
    broadcast_message<true>(login_payload, conn.user()->id);

    send_feedback<true, FeedbackId::Login>(symm_encryptor, conn);
  }

  void Server::register_message_handler(Connection& conn, const Message& msg) noexcept
  {
    auto header = msg.as_header();

    auto& symm_encryptor = conn.symmetric_encryptor();
    if (!expect_prologue<false, Message::EncryptionType::Symmetric, FeedbackId::Register>(
        symm_encryptor, conn, *header))
      return;

    auto payload = get_payload<RegisterPayload>(symm_encryptor, msg);
    if (!payload)
    {
      Logger::error(fmt::format("Failed to deserialize connection-{} payload: {}", conn.id(),
                                payload.error()));
      send_feedback<false, FeedbackId::Register>(symm_encryptor, conn, MESSAGE_MALFORMED);
      return;
    }

    // Check if user already exists
    const auto user = std::ranges::find_if(
        users_, [username = std::string_view{payload->username}](const std::unique_ptr<User>& usr) {
          return username == usr->name;
        });

    // user with username already exists
    if (user != users_.end())
    {
      Logger::warn(fmt::format("Connection-{} failed on registering with username: {}",
                               conn.id(), payload->username));
      send_feedback<false, FeedbackId::Register>(symm_encryptor, conn, USER_NOT_FOUND);
      return;
    }

    Logger::info(fmt::format("Connection-{} registered successfully with username: {}",
                             conn.id(), std::string_view{payload->username}));

    users_.emplace_back(std::make_unique<User>(user_id_generator_.gen(),
                                               std::move(payload->username),
                                               std::move(payload->password), std::vector<u8>{}));

    send_feedback<true, FeedbackId::Register>(symm_encryptor, conn);
  }

  void Server::get_user_online_handler(Connection& conn, const Message& msg) noexcept
  {
    auto header = msg.as_header();
    auto& symm_encryptor = conn.symmetric_encryptor();
    if (!expect_prologue<true, Message::EncryptionType::Symmetric, FeedbackId::GetUserOnline>(
        symm_encryptor, conn, *header))
      return;

    // filter online users
    std::vector<UserResponse> users;
    for (const auto& con : connections_)
    {
      if (!con->user() || con->user()->id == conn.user()->id)
        continue;
      users.emplace_back(con->user()->id, con->user()->name);
    }

    UserOnlinePayload payload{.users = std::move(users)};
    send_message(symm_encryptor, conn, payload);
  }

  void Server::get_user_details_handler(Connection& conn, const Message& msg) noexcept
  {
    auto header = msg.as_header();
    auto symm_encryptor = conn.symmetric_encryptor();
    if (!expect_prologue<true, Message::EncryptionType::Symmetric, FeedbackId::GetUserDetails>(
        symm_encryptor, conn, *header))
      return;

    auto payload = get_payload<GetUserDetailsPayload>(symm_encryptor, msg);
    if (!payload)
    {
      Logger::error(fmt::format("Connection-{} sent malformed packet: {}", conn.id(),
                                payload.error()));
      send_feedback<false, FeedbackId::GetUserDetails>(symm_encryptor, conn, MESSAGE_MALFORMED);
      return;
    }

    // Get the user
    auto it = std::ranges::find_if(
        users_, [&](const std::unique_ptr<User>& user) {
          return user->id == payload->id;
        });

    if (it == users_.end())
    {
      Logger::warn(fmt::format("Connection-{} trying to get details of non-existent user",
                               conn.id()));
      send_feedback<false, FeedbackId::GetUserDetails>(symm_encryptor, conn, USER_NOT_FOUND);
      return;
    }

    UserDetailPayload resp_payload{.id = it->get()->id,
                                   .username = it->get()->name,
                                   .public_key = it->get()->public_key};

    send_message(symm_encryptor, conn, resp_payload);
  }

  void Server::store_symmetric_key_handler(Connection& conn, const Message& msg) noexcept
  {
    // Payload encrypted using asymmetric but not for response
    auto header = msg.as_header();
    if (!expect_prologue<false, Message::EncryptionType::Asymmetric, FeedbackId::StoreSymmetricKey>(
        conn, *header))
      return;

    auto decipher = asymm_encryptor_.decrypts(msg.body);
    if (!decipher)
    {
      send_feedback<false, FeedbackId::StoreSymmetricKey>(conn, MESSAGE_MALFORMED);
      return;
    }

    auto data_bytes = ar::as_byte_span<asymm_type::block_type>(
        decipher.value(), header->body_filler);

    auto payload = parse_body<StoreSymmetricKeyPayload>(data_bytes);
    if (!payload)
    {
      send_feedback<false, FeedbackId::StoreSymmetricKey>(conn, MESSAGE_MALFORMED);
      return;
    }

    if (payload->key.size() != KEY_BYTE)
    {
      send_feedback<false, FeedbackId::StoreSymmetricKey>(conn, MESSAGE_MALFORMED);
      return;
    }

    // set symmetric key into connection
    conn.symmetric_encryptor(symm_type::key_type{payload->key});

    Logger::trace(fmt::format("sending {} response packet to connection-{}",
                              magic_enum::enum_name<Message::Type::StoreSymmetricKey>(),
                              conn.id()));

    send_feedback<true, FeedbackId::StoreSymmetricKey>(conn);
  }

  void Server::get_server_details_handler(Connection& conn, const Message& msg) noexcept
  {
    // All data unencrypted
    auto header = msg.as_header();

    if (expect_prologue<false, Message::EncryptionType::None, FeedbackId::GetServerDetails>(
        conn, *header))
      return;

    auto public_key = serialize(asymm_encryptor_.public_key());
    ServerDetailsPayload details_payload{
        .id = User::SERVER_ID,
        .public_key = public_key
    };

    Logger::trace(fmt::format("sending {} response packet to user {}",
                              magic_enum::enum_name<Message::Type::GetServerDetail>(),
                              std::string_view{conn.user()->name}));

    send_message(conn, details_payload);
  }

  void Server::store_public_key_handler(Connection& conn, const Message& msg) noexcept
  {
    // Encrypted using symmetric key
    auto header = msg.as_header();

    auto& symm_encryptor = conn.symmetric_encryptor();
    if (!expect_prologue<true, Message::EncryptionType::Symmetric, FeedbackId::StorePublicKey>(
        symm_encryptor, conn, *header))
      return;

    auto payload = get_payload<StorePublicKeyPayload>(symm_encryptor, msg);
    if (!payload)
    {
      Logger::warn(fmt::format(
          "Connection-{} sent malformed {} packet", conn.id(),
          magic_enum::enum_name<Message::Type::StorePublicKey>()
          ));
      send_feedback<false, FeedbackId::StorePublicKey>(symm_encryptor, conn, payload.error());
      return;
    }

    // save public key
    conn.user()->public_key = std::move(payload->key);

    Logger::info(fmt::format("sending {} response packet to user {}",
                             magic_enum::enum_name<Message::Type::StorePublicKey>(),
                             std::string_view{conn.user()->name}));

    send_feedback<true, FeedbackId::StorePublicKey>(symm_encryptor, conn);
  }

  void Server::send_file_handler(Connection& conn, const Message& msg) noexcept
  {
    auto header = msg.as_header();
    if (!expect_prologue<true, Message::EncryptionType::None, FeedbackId::SendFile>(conn, *header))
      return;

    // check if user opponent is online
    auto& symm_encryptor = conn.symmetric_encryptor();
    if (!user_connections_.contains(header->opponent_id))
    {
      Logger::warn(fmt::format(
          "User-{} trying to send file into user with id {}, which doesn't exists",
          conn.user()->name, header->opponent_id));
      if constexpr (AR_DEBUG)
      {
        // Show registered and online users
        std::vector<User::id_type> keys{};
        for (const auto& k : user_connections_ | std::views::keys)
          keys.emplace_back(k);

        Logger::info(fmt::format("[DEBUG] Online Users: {}", keys));
      }
      send_feedback<false, FeedbackId::SendFile>(symm_encryptor, conn, USER_ID_NOT_FOUND);
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
          fmt::format("User-{}[{}] sending files to user-{}", conn.user()->name, conn.id(),
                      user->get()->name));
    }

    // Send to all clients connected to specific user
    for (auto& connId : connections)
    {
      const auto con = std::ranges::find_if(
          connections_,
          [=](const std::weak_ptr<Connection> conn) {
            return conn.lock()->id() == connId;
          });

      if (con == connections_.end())
      {
        Logger::warn(
            fmt::format("user connections has id that not belongs to any client: {}", connId));
        continue;
      }

      // copy message and change the opponent id into sender id
      auto msg_copy = msg;
      msg_copy.as_header()->opponent_id = conn.user()->id;
      // con->get()->write(std::move(msg_copy));
      send_message(*con->get(), std::move(msg_copy));
    }

    send_feedback<true, FeedbackId::SendFile>(symm_encryptor, conn);
  }
} // namespace ar