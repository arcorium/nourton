#pragma once

#include <alpaca/alpaca.h>
#include <fmt/format.h>

#include <array>
#include <expected>
#include <span>
#include <vector>

#include "message.h"
#include "feedback.h"
#include "user.h"
#include "util/types.h"
#include "logger.h"
#include "crypto/hybrid.h"

namespace ar
{
  struct UserDetailPayload
  {
    User::id_type id;
    std::string username;
    std::vector<u8> public_key;

    [[nodiscard]] std::vector<u8> serialize() const noexcept
    {
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);
      return temp;
    }
  };

  struct GetUserDetailsPayload
  {
    User::id_type id;

    [[nodiscard]] std::vector<u8> serialize() const noexcept
    {
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);
      return temp;
    }
  };

  struct GetServerDetailsPayload
  {
    [[nodiscard]] std::vector<u8> serialize() const noexcept
    {
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);
      return temp;
    }
  };

  struct ServerDetailsPayload
  {
    User::id_type id;
    std::vector<u8> public_key;

    [[nodiscard]] std::vector<u8> serialize() const noexcept
    {
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);
      return temp;
    }
  };

  struct GetUserOnlinePayload
  {
    // Empty struct
    [[nodiscard]] std::vector<u8> serialize() const noexcept
    {
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);
      return temp;
    }
  };

  struct UserOnlinePayload
  {
    std::vector<UserResponse> users;

    [[nodiscard]] std::vector<u8> serialize() const noexcept
    {
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);
      return temp;
    }
  };

  struct LoginPayload
  {
    std::string username;
    std::string password;

    [[nodiscard]] std::vector<u8> serialize() const noexcept
    {
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);
      return temp;
    }
  };

  struct RegisterPayload
  {
    std::string username;
    std::string password;

    [[nodiscard]] std::vector<u8> serialize() const noexcept
    {
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);
      return temp;
    }
  };

  struct StorePublicKeyPayload
  {
    std::vector<u8> key;

    [[nodiscard]] std::vector<u8> serialize() const noexcept
    {
      std::vector<u8> temp;
      alpaca::serialize(*this, temp);
      return temp;
    }
  };

  struct StoreSymmetricKeyPayload
  {
    std::vector<u8> key;

    [[nodiscard]] std::vector<u8> serialize() const noexcept
    {
      std::vector<u8> temp;
      alpaca::serialize(*this, temp);
      return temp;
    }
  };

  struct SendFilePayload
  {
    u8 file_filler;
    u8 key_filler;
    u64 file_size;
    std::string filename;
    std::string timestamp;
    std::vector<u8> symmetric_key; // encrypted
    std::vector<u8> files;

    [[nodiscard]] std::vector<u8> serialize() const noexcept
    {
      std::vector<u8> temp;
      alpaca::serialize(*this, temp);
      return temp;
    }
  };

  struct UserLoginPayload
  {
    User::id_type id;
    std::string username;

    [[nodiscard]] std::vector<u8> serialize() const noexcept
    {
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);
      return temp;
    }
  };

  struct UserLogoutPayload
  {
    User::id_type id;

    [[nodiscard]] std::vector<u8> serialize() const noexcept
    {
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);
      return temp;
    }
  };

  template <Message::Type MsgType, Message::EncryptionType EncryptType =
                Message::EncryptionType::None>
  static constexpr Message create_message(std::span<u8> payload,
                                          Message::Header::opponent_id_type opponent_id,
                                          u16 filler) noexcept
  {
    auto header = Message::Header{
        .body_size = payload.size_bytes(),
        .body_filler = filler,
        .encryption = EncryptType,
        .message_type = MsgType,
        .opponent_id = opponent_id,
    };

    return Message{header, payload};
  }

  template <typename T>
  consteval Message::Type get_payload_type() noexcept
  {
    if constexpr (std::same_as<T, LoginPayload>)
    {
      return Message::Type::Login;
    }
    if constexpr (std::same_as<T, RegisterPayload>)
    {
      return Message::Type::Register;
    }
    if constexpr (std::same_as<T, StorePublicKeyPayload>)
    {
      return Message::Type::StorePublicKey;
    }
    if constexpr (std::same_as<T, StoreSymmetricKeyPayload>)
    {
      return Message::Type::StoreSymmetricKey;
    }
    if constexpr (std::same_as<T, GetUserOnlinePayload>)
    {
      return Message::Type::GetUserOnline;
    }
    if constexpr (std::same_as<T, UserOnlinePayload>)
    {
      return Message::Type::UserOnlineResponse;
    }
    if constexpr (std::same_as<T, GetUserDetailsPayload>)
    {
      return Message::Type::GetUserDetails;
    }
    if constexpr (std::same_as<T, UserDetailPayload>)
    {
      return Message::Type::UserDetailsResponse;
    }
    if constexpr (std::same_as<T, GetServerDetailsPayload>)
    {
      return Message::Type::GetServerDetail;
    }
    if constexpr (std::same_as<T, ServerDetailsPayload>)
    {
      return Message::Type::ServerDetailResponse;
    }
    if constexpr (std::same_as<T, SendFilePayload>)
    {
      return Message::Type::SendFile;
    }
    if constexpr (std::same_as<T, UserLoginPayload>)
    {
      return Message::Type::UserLogin;
    }
    if constexpr (std::same_as<T, UserLogoutPayload>)
    {
      return Message::Type::UserLogout;
    }
    if constexpr (std::same_as<T, FeedbackPayload>)
    {
      return Message::Type::Feedback;
    }

    return static_cast<Message::Type>(std::numeric_limits<std::underlying_type_t<
      Message::Type>>::max());
  }
} // namespace ar