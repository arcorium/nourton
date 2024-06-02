#pragma once
#include <magic_enum.hpp>
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
    using asymm_type = DMRSA;

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

    template <bool Expect, FeedbackId Id>
    bool expect_auth(Connection& conn AR_SOURCE_LOCATION_PARAM_2) noexcept;

    // It will return bad feedback response which is encrypted
    template <bool Expect, FeedbackId Id>
    bool expect_auth(symm_type& symm_encryptor,
                     Connection& conn AR_SOURCE_LOCATION_PARAM_2) noexcept;

    template <Message::EncryptionType Expect, FeedbackId Id>
    bool expect_encryption(const Message::Header& header,
                           Connection& conn AR_SOURCE_LOCATION_PARAM_2) noexcept;

    template <Message::EncryptionType Expect, FeedbackId Id>
    bool expect_encryption(symm_type& symm_encryptor, const Message::Header& header,
                           Connection& conn AR_SOURCE_LOCATION_PARAM_2) noexcept;

    template <bool ExpectAuth, Message::EncryptionType ExpectEnc, FeedbackId Id>
    bool expect_prologue(Connection& conn,
                         const Message::Header& header AR_SOURCE_LOCATION_PARAM_2) noexcept;

    template <bool ExpectAuth, Message::EncryptionType ExpectEnc, FeedbackId Id>
    bool expect_prologue(symm_type& symm_encryptor, Connection& conn,
                         const Message::Header& header AR_SOURCE_LOCATION_PARAM_2) noexcept;

    // Not encrypted
    template <bool Resp, FeedbackId Id>
    void send_feedback(Connection& conn, std::string_view message = ""sv) noexcept;

    // Encrypted using symmetric
    template <bool Resp, FeedbackId Id>
    void send_feedback(symm_type& symm_encryptor, Connection& conn,
                       std::string_view message = ""sv) noexcept;

    template <payload T>
    std::expected<T, std::string_view> get_payload(const Message& msg) noexcept;

    template <payload T>
    std::expected<T, std::string_view> get_payload(symm_type& symm_encryptor,
                                                   const Message& msg) noexcept;

    template <payload T>
    void send_message(Connection& conn, const T& payload) noexcept;

    template <payload T>
    void send_message(symm_type& symm_encryptor, Connection& conn, const T& payload) noexcept;

    // When Encrypt is true, the message will be encrypted by each cliet symmetric key
    template <bool Encrypt, payload T>
    void broadcast_message(const T& payload, User::id_type except_id) noexcept;

    // handler
    void login_message_handler(Connection& conn, const Message& msg) noexcept;
    void register_message_handler(Connection& conn, const Message& msg) noexcept;
    void get_user_online_handler(Connection& conn, const Message& msg) noexcept;
    void get_user_details_handler(Connection& conn, const Message& msg) noexcept;
    void store_symmetric_key_handler(Connection& conn, const Message& msg) noexcept;
    void get_server_details_handler(Connection& conn, const Message& msg) noexcept;
    void store_public_key_handler(Connection& conn, const Message& msg) noexcept;
    void send_file_handler(Connection& conn, const Message& msg) noexcept;

  private:
    asio::any_io_executor executor_;
    asio::strand<asio::any_io_executor> strand_;
    asio::ip::tcp::acceptor acceptor_;

    std::vector<std::shared_ptr<Connection>> connections_; // client database
    std::map<u16, std::vector<u16>> user_connections_;
    std::vector<std::unique_ptr<User>> users_; // user database
    IdGenerator<u16> user_id_generator_;

    asymm_type asymm_encryptor_; // only used for key exchange
  };

  template <bool Expect, FeedbackId Id>
  bool Server::expect_auth(Connection& conn AR_SOURCE_LOCATION_PARAM_2_DECL)
    noexcept
  {
    if (conn.is_authenticated() == Expect)
      return true;

    if constexpr (Expect)
    {
      Logger::warn(fmt::format(
                       "Connection-{} is unauthenticated while trying to do some actions",
                       conn.id()) AR_SOURCE_LOCATION_VAR_2);
    }
    else
    {
      Logger::warn(fmt::format(
                       "Connection-{} already authenticated, which is prohibited",
                       conn.id()) AR_SOURCE_LOCATION_VAR_2);
    }
    send_feedback<false, Id>(conn, Expect ? UNAUTHENTICATED_MESSAGE : AUTHENTICATED_MESSAGE);
    return false;
  }

  template <bool Expect, FeedbackId Id>
  bool Server::expect_auth(symm_type& symm_encryptor,
                           Connection& conn AR_SOURCE_LOCATION_PARAM_2_DECL)
    noexcept
  {
    if (conn.is_authenticated() == Expect)
      return true;

    if constexpr (Expect)
    {
      Logger::warn(fmt::format(
          "Connection-{} is unauthenticated while trying to get server details", conn.id()));
    }
    else
    {
      Logger::warn(fmt::format(
          "Connection-{} already authenticated, which is prohibited", conn.id()));
    }
    send_feedback<false, Id>(symm_encryptor, conn,
                             Expect ? UNAUTHENTICATED_MESSAGE : AUTHENTICATED_MESSAGE);
    return false;
  }

  template <Message::EncryptionType Expect, FeedbackId Id>
  bool Server::expect_encryption(const Message::Header& header,
                                 Connection& conn AR_SOURCE_LOCATION_PARAM_2_DECL) noexcept
  {
    if (header.encryption == Expect)
      return true;

    if constexpr (Expect == Message::EncryptionType::None)
    {
      Logger::warn(fmt::format(
                       "Connection-{} sent {} packet instead of unencrypted", conn.id(),
                       magic_enum::enum_name<Expect>()) AR_SOURCE_LOCATION_VAR_2);

      send_feedback<false, Id>(conn, MESSAGE_SHOULD_NOT_ENCRYPTED);
    }
    else if constexpr (Expect == Message::EncryptionType::Asymmetric)
    {
      Logger::warn(fmt::format(
                       "Connection-{} sent {} encrypted packet instead of asymmetric", conn.id(),
                       magic_enum::enum_name<Expect>()) AR_SOURCE_LOCATION_VAR_2);

      send_feedback<false, Id>(conn, MESSAGE_ENCRYPTION_IS_NOT_ASYMMETRIC);
    }
    else if constexpr (Expect == Message::EncryptionType::Symmetric)
    {
      Logger::warn(fmt::format(
                       "Connection-{} sent {} encrypted packet instead of symmetric", conn.id(),
                       magic_enum::enum_name<Expect>()) AR_SOURCE_LOCATION_VAR_2);

      send_feedback<false, Id>(conn, MESSAGE_ENCRYPTION_IS_NOT_SYMMETRIC);
    }

    return false;
  }

  template <Message::EncryptionType Expect, FeedbackId Id>
  bool Server::expect_encryption(symm_type& symm_encryptor, const Message::Header& header,
                                 Connection& conn AR_SOURCE_LOCATION_PARAM_2_DECL) noexcept
  {
    if (header.encryption == Expect)
      return true;

    if constexpr (Expect == Message::EncryptionType::None)
    {
      Logger::warn(fmt::format(
                       "Connection-{} sent {} packet instead of unencrypted", conn.id(),
                       magic_enum::enum_name<Expect>()) AR_SOURCE_LOCATION_VAR_2);

      send_feedback<false, Id>(symm_encryptor, conn, MESSAGE_SHOULD_NOT_ENCRYPTED);
    }
    else if constexpr (Expect == Message::EncryptionType::Asymmetric)
    {
      Logger::warn(fmt::format(
                       "Connection-{} sent {} encrypted packet instead of asymmetric", conn.id(),
                       magic_enum::enum_name<Expect>()) AR_SOURCE_LOCATION_VAR_2);

      send_feedback<false, Id>(symm_encryptor, conn, MESSAGE_ENCRYPTION_IS_NOT_ASYMMETRIC);
    }
    else if constexpr (Expect == Message::EncryptionType::Symmetric)
    {
      Logger::warn(fmt::format(
                       "Connection-{} sent {} encrypted packet instead of symmetric", conn.id(),
                       magic_enum::enum_name<Expect>()) AR_SOURCE_LOCATION_VAR_2);

      send_feedback<false, Id>(symm_encryptor, conn, MESSAGE_ENCRYPTION_IS_NOT_SYMMETRIC);
    }

    return false;
  }

  template <bool ExpectAuth, Message::EncryptionType ExpectEnc, FeedbackId Id>
  bool Server::expect_prologue(Connection& conn,
                               const Message::Header& header AR_SOURCE_LOCATION_PARAM_2_DECL)
    noexcept
  {
    return expect_auth<ExpectAuth, Id>(conn
                                       AR_SOURCE_LOCATION_VAR_2
               )
           && expect_encryption<
             ExpectEnc, Id>(header, conn
                            AR_SOURCE_LOCATION_VAR_2
               );
  }

  template <bool ExpectAuth, Message::EncryptionType ExpectEnc, FeedbackId Id>
  bool Server::expect_prologue(symm_type& symm_encryptor, Connection& conn,
                               const Message::Header& header AR_SOURCE_LOCATION_PARAM_2_DECL)
    noexcept
  {
    return expect_auth<ExpectAuth, Id>(symm_encryptor, conn
                                       AR_SOURCE_LOCATION_VAR_2
               )
           &&
           expect_encryption<ExpectEnc, Id>(symm_encryptor, header, conn
                                            AR_SOURCE_LOCATION_VAR_2
               );
  }

  template <bool Resp, FeedbackId Id>
  void Server::send_feedback(Connection& conn, std::string_view message) noexcept
  {
    FeedbackPayload resp_payload{
        .id = Id,
        .response = Resp,
        .message = std::string{message},
    };
    auto serialized = resp_payload.serialize();
    auto msg = create_message<Message::Type::Feedback>(serialized, User::SERVER_ID, 0);
    conn.write(std::move(msg));
  }

  template <bool Resp, FeedbackId Id>
  void Server::send_feedback(symm_type& symm_encryptor, Connection& conn,
                             std::string_view message) noexcept
  {
    FeedbackPayload resp_payload{
        .id = Id,
        .response = Resp,
        .message = std::string{message},
    };
    auto serialized = resp_payload.serialize();

    // Encrypt
    auto [filler, cipher] = symm_encryptor.encrypts(serialized);

    auto msg = create_message<Message::Type::Feedback, Message::EncryptionType::Symmetric>(
        cipher, User::SERVER_ID, filler);
    conn.write(std::move(msg));
  }

  template <payload T>
  std::expected<T, std::string_view> Server::get_payload(const Message& msg) noexcept
  {
    return msg.body_as<T>();
  }

  template <payload T>
  std::expected<T, std::string_view> Server::get_payload(symm_type& symm_encryptor,
                                                         const Message& msg) noexcept
  {
    auto header = msg.as_header();
    // decrypt
    auto result = symm_encryptor.decrypts(msg.body, header->body_filler);
    if (!result)
      return std::unexpected{result.error()};

    // FIX: not needed
    if (result->size() != header->real_size())
      return std::unexpected{MESSAGE_MALFORMED};

    return parse_body<T>(result.value());
  }

  template <payload T>
  void Server::send_message(Connection& conn, const T& payload) noexcept
  {
    constexpr auto payload_type = get_payload_type<T>();

    auto serialized = payload.serialize();
    auto resp_msg = create_message<payload_type>(serialized, User::SERVER_ID, 0);
    conn.write(std::move(resp_msg));
  }

  template <payload T>
  void Server::send_message(symm_type& symm_encryptor, Connection& conn, const T& payload) noexcept
  {
    constexpr auto payload_type = get_payload_type<T>();

    auto serialized = payload.serialize();
    auto [padding, cipher] = symm_encryptor.encrypts(serialized);
    auto resp_msg = create_message<payload_type, Message::EncryptionType::Symmetric>(
        cipher, User::SERVER_ID, padding);
    conn.write(std::move(resp_msg));
  }

  template <bool Encrypt, payload T>
  void Server::broadcast_message(const T& payload, User::id_type except) noexcept
  {
    Logger::trace(fmt::format("Broadcasting encrypted message except for client {}", except));

    constexpr auto payload_type = get_payload_type<T>();
    auto serialized = payload.serialize();
    // encrypt

    for (const auto& conn : connections_)
    {
      // ignore unauthenticated connection and sender
      if (!conn->user() || conn->user()->id == except)
        continue;

      if constexpr (Encrypt)
      {
        auto& symm_encryptor = conn->symmetric_encryptor();
        auto [padding, cipher] = symm_encryptor.encrypts(serialized);
        auto msg = create_message<payload_type, Message::EncryptionType::Symmetric>(
            cipher, User::SERVER_ID, padding);
        conn->write(std::move(msg));
      }
      else
      {
        auto msg = create_message<payload_type>(serialized, User::SERVER_ID, 0);
        conn->write(std::move(msg));
      }
    }
  }
} // namespace ar