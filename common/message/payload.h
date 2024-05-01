#pragma once

#include "../util/make.h"
#include "../util/types.h"

#include <array>
#include <expected>
#include <vector>
#include <span>

#include <alpaca/alpaca.h>

#include <fmt/format.h>

#include "logger.h"

#include "../user.h"

namespace ar
{
  struct Message
  {
    enum class Type : u8
    {
      Login,
      Register,
      StorePublicKey,
      GetUserOnline,
      UserOnlineResponse,
      GetUserDetails,
      UserDetailsResponse,
      SendFile,
      // ---Signal
      UserLogout,
      UserLogin,
      // ---Signal
      Feedback
    };

    struct Header
    {
      u64 body_size;
      Type message_type;
      u16 opponent_id; // 0 = server
    };

    constexpr static u8 header_size = sizeof(Header);

    // TODO: Remove default constructor to force header initialization
    Message() :
      header{}, body{} {}

    explicit Message(const Header& header_) noexcept
      : header{}, body{}
    {
      std::memcpy(header.data(), &header_, header_size);
    }

    // WARN: header should not be serialized using alpaca, use the underlying memory layout of Header struct instead
    std::array<u8, header_size> header;
    std::vector<u8> body;

    [[nodiscard]]
    Header parse_header() const noexcept
    {
      Header temp{};
      std::memcpy(&temp, header.data(), header_size);
      return temp;
    }

    [[nodiscard]] Header const* as_header() const noexcept
    {
      return reinterpret_cast<Header const*>(header.data());
    }

    // TODO: change return into std::expected instead
    template <typename T>
    [[nodiscard]] T body_as() const noexcept
    {
      std::error_code ec;
      auto result = alpaca::deserialize<T>(body, ec);
      if (ec)
      {
        Logger::warn(fmt::format("failed to deserialize to type {}", typeid(T).name()));
      }
      return result;
    }

    [[nodiscard]] usize size() const noexcept
    {
      return header_size + body.size();
    }
  };

  struct UserDetailPayload
  {
    User::id_type id;
    std::string username;
    std::vector<u8> public_key;

    [[nodiscard]] Message serialize() const noexcept
    {
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);

      Message::Header header{
        .body_size = temp.size(),
        .message_type = Message::Type::UserDetailsResponse,
        .opponent_id = 0,
      };

      Message message{};
      std::memcpy(message.header.data(), &header, Message::header_size);
      message.body = std::move(temp);

      return message;
    }
  };

  struct GetUserDetailsPayload
  {
    User::id_type id;

    [[nodiscard]] Message serialize() const noexcept
    {
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);

      Message::Header header{
        .body_size = temp.size(),
        .message_type = Message::Type::GetUserDetails,
        .opponent_id = 0,
      };

      Message message{};
      std::memcpy(message.header.data(), &header, Message::header_size);
      message.body = std::move(temp);

      return message;
    }
  };

  struct GetUserOnlinePayload
  {
    // Empty struct
    [[nodiscard]] Message serialize() const noexcept
    {
      // PERF: maybe it is not needed
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);

      Message::Header header{
        .body_size = temp.size(),
        .message_type = Message::Type::GetUserOnline,
        .opponent_id = 0,
      };

      Message message{};
      // alpaca::serialize(header, message.header);
      std::memcpy(message.header.data(), &header, Message::header_size);
      message.body = std::move(temp);

      return message;
    }
  };

  struct UserOnlinePayload
  {
    std::vector<UserResponse> users;

    [[nodiscard]] Message serialize() const noexcept
    {
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);

      Message::Header header{
        .body_size = temp.size(),
        .message_type = Message::Type::UserOnlineResponse,
        .opponent_id = 0,
      };

      Message message{};
      // alpaca::serialize(header, message.header);
      std::memcpy(message.header.data(), &header, Message::header_size);
      message.body = std::move(temp);

      return message;
    }
  };

  struct LoginPayload
  {
    std::string username;
    std::string password;

    [[nodiscard]] Message serialize() const noexcept
    {
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);

      Message::Header header{
        .body_size = temp.size(),
        .message_type = Message::Type::Login,
        .opponent_id = 0,
      };

      Message message{};
      // alpaca::serialize(header, message.header);
      std::memcpy(message.header.data(), &header, Message::header_size);
      message.body = std::move(temp);

      return message;
    }
  };

  struct RegisterPayload
  {
    std::string username;
    std::string password;

    [[nodiscard]] Message serialize() const noexcept
    {
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);

      Message::Header header{
        .body_size = temp.size(),
        .message_type = Message::Type::Register,
        .opponent_id = 0,
      };

      Message message{};
      // alpaca::serialize(header, message.header);
      std::memcpy(message.header.data(), &header, Message::header_size);
      message.body = std::move(temp);

      return message;
    }
  };

  struct StorePublicKeyPayload
  {
    std::vector<u8> public_key;

    [[nodiscard]] Message serialize() const noexcept
    {
      std::vector<u8> temp;
      alpaca::serialize(*this, temp);

      Message::Header header{
        .body_size = temp.size(),
        .message_type = Message::Type::StorePublicKey,
        .opponent_id = 0,
      };
      Message msg{};
      // alpaca::serialize(header, msg.header);
      std::memcpy(msg.header.data(), &header, Message::header_size);
      msg.body = std::move(temp);

      return msg;
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

    [[nodiscard]] Message serialize(User::id_type id) const noexcept
    {
      std::vector<u8> temp;
      alpaca::serialize(*this, temp);

      Message::Header header{
        .body_size = temp.size(),
        .message_type = Message::Type::SendFile,
        .opponent_id = id,
      };
      Message msg{};
      // alpaca::serialize(header, msg.header);
      std::memcpy(msg.header.data(), &header, Message::header_size);
      msg.body = std::move(temp);

      return msg;
    }
  };

  struct UserLoginPayload
  {
    User::id_type id;
    std::string username;

    [[nodiscard]] Message serialize() const noexcept
    {
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);

      Message::Header header{
        .body_size = temp.size(),
        .message_type = Message::Type::UserLogin,
        .opponent_id = 0,
      };

      Message message{};
      // alpaca::serialize(header, message.header);
      std::memcpy(message.header.data(), &header, Message::header_size);
      message.body = std::move(temp);

      return message;
    }
  };

  struct UserLogoutPayload
  {
    User::id_type id;

    [[nodiscard]] Message serialize() const noexcept
    {
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);

      Message::Header header{
        .body_size = temp.size(),
        .message_type = Message::Type::UserLogout,
        .opponent_id = 0,
      };

      Message message{};
      // alpaca::serialize(header, message.header);
      std::memcpy(message.header.data(), &header, Message::header_size);
      message.body = std::move(temp);

      return message;
    }
  };

  // Feedback for specific request payload
  enum class PayloadId : u8
  {
    Login,
    Register,
    GetUserOnline,
    GetUserDetails,
    StorePublicKey,
    SendFile
  };

  static constexpr std::string_view UNAUTHENTICATED_MESSAGE{"you need to authenticate first to do this action"};
  static constexpr std::string_view AUTHENTICATED_MESSAGE{"you need to logout first to do this action"};

  struct FeedbackPayload
  {
    PayloadId id;
    bool response;
    std::optional<std::string> message;

    [[nodiscard]] Message serialize(User::id_type sender_id = 0) const noexcept
    {
      std::vector<u8> temp;
      alpaca::serialize(*this, temp);

      auto header = Message::Header{
        .body_size = temp.size(),
        .message_type = Message::Type::Feedback,
        .opponent_id = sender_id
      };

      auto msg = Message{};
      // alpaca::serialize(header, msg.header);
      std::memcpy(msg.header.data(), &header, Message::header_size);
      msg.body = std::move(temp);
      return msg;
    }
  };
}
