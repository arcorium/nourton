#pragma once

#include <asio/awaitable.hpp>
#include <asio/execution_context.hpp>
#include <asio/ip/tcp.hpp>
#include <string_view>

#include "connection.h"
#include "crypto/hybrid.h"
#include "util/asio.h"
#include "util/types.h"

namespace ar
{
  class IEventHandler;

  class Client
  {
    using asymm_enc = DMRSA;
    using symm_enc = Camellia;

  public:
    Client(asio::any_io_executor executor, asio::ip::address address, u16 port,
           IEventHandler* event_handler) noexcept;

    Client(asio::any_io_executor executor, asio::ip::tcp::endpoint endpoint,
           IEventHandler* event_handler) noexcept;

    ~Client() noexcept;

    Client(const Client&) = delete;

    Client& operator=(const Client&) = delete;

    Client(Client&& other) noexcept;

    Client& operator=(Client&&) = delete;

    asio::awaitable<bool> start() noexcept;

    void disconnect() noexcept;

    bool is_connected() const noexcept;

    // send payload and encrypt it with asymmetric encryption
    template <serializable T>
    void write(const asymm_enc::_public_key& pk, T&& payload,
               Message::Header::opponent_id_type opponent_id) noexcept;

    // send payload with conditional symmetric encrypting.
    // if the Encrypt is false, then no encrypting process will be applied on the payload
    // which is currently used for send file payload.
    template <serializable T, bool Encrypt = true>
    void write(T&& payload, Message::Header::opponent_id_type opponent_id) noexcept;

    bool send_file(const asymm_enc::_public_key& pk, std::string_view filepath,
                   User::id_type opponent_id) noexcept;

    template <typename Self>
    auto&& executor(this Self&& self) noexcept;

    asymm_type& asymmetric_encryptor() noexcept;

    symm_type& symmetric_encryptor() noexcept;

  private:
    template <payload T>
    std::expected<T, std::string_view> get_payload(const Message& msg) noexcept;

    asio::awaitable<void> reader() noexcept;

    void message_handler(const Message& msg) noexcept;

  private:
    IEventHandler* event_handler_;
    asio::any_io_executor executor_;
    asio::ip::tcp::endpoint endpoint_;

    Connection connection_;
    // TODO: Move all encrypting thingies in here instead of on application
    DMRSA asymm_encryptor_;
    Camellia symm_encryptor_;
  };

  template <typename Self>
  auto&& Client::executor(this Self&& self) noexcept
  {
    return std::forward<Self>(self).executor_;
  }

  template <serializable T>
  void Client::write(const asymm_enc::_public_key& pk, T&& payload,
                     Message::Header::opponent_id_type opponent_id) noexcept
  {
    constexpr auto type = get_payload_type<T>();
    auto body = payload.serialize();

    asymm_type asymm{pk};
    auto [filler, cipher] = asymm.encrypts(body);
    auto cipher_bytes = ar::as_byte_span<asymm_type::block_enc_type>(cipher);

    auto msg = create_message<type, Message::EncryptionType::Asymmetric>(
        cipher_bytes, opponent_id, filler);
    connection_.write(std::move(msg));
  }

  template <serializable T, bool Encrypt>
  void Client::write(T&& payload, Message::Header::opponent_id_type opponent_id) noexcept
  {
    constexpr auto type = get_payload_type<T>();
    auto body = payload.serialize();

    Message msg;
    if constexpr (Encrypt)
    {
      auto [filler, cipher] = symm_encryptor_.encrypts(body);
      msg = create_message<type, Message::EncryptionType::Symmetric>(cipher, opponent_id, filler);
    }
    else
    {
      msg = create_message<type>(body, opponent_id, 0);
    }
    connection_.write(std::move(msg));
  }

  template <payload T>
  std::expected<T, std::string_view> Client::get_payload(const Message& msg) noexcept
  {
    constexpr auto type = get_payload_type<T>();
    const auto header = msg.as_header();

    // decrypt using symmetric
    if (header->encryption == Message::EncryptionType::Symmetric)
    {
      auto result = symm_encryptor_.decrypts(msg.body, header->body_filler);
      if (!result)
        return std::unexpected(result.error());

      return parse_body<T>(result.value());
    }

    if (header->encryption == Message::EncryptionType::Asymmetric)
    {
      auto result = asymm_encryptor_.decrypts(msg.body);
      if (!result)
        return std::unexpected(result.error());

      auto bytes = as_byte_span<asymm_type::block_type>(result.value(), header->body_filler);
      return parse_body<T>(bytes);
    }

    // handle when the payload is not encrypted
    return msg.body_as<T>();
  }
} // namespace ar