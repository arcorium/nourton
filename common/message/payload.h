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
      GetUserOnline,
      GetUserDetails,
      SendFile,
      Feedback
    };

    struct Header
    {
      u64 body_size;
      Type message_type;
      u16 opponent_id;
    };

    Message() :
      header{}, body{}
    {
    }

    constexpr static u8 header_size = sizeof(Header);
    std::array<u8, header_size> header; //TODO: Combine header and body, so it doesn't need to combine it later
    std::vector<u8> body;

    [[nodiscard]]
    Header parse_header() const noexcept
    {
      Header temp{};
      std::memcpy(&temp, header.data(), header_size);
      return temp;
    }


    [[nodiscard]] Header const* get_header() const noexcept
    {
      return reinterpret_cast<Header const*>(header.data());
    }

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
  };

  struct LoginPayload
  {
    std::string username;
    std::string password;

    Message serialize() const noexcept
    {
      std::vector<u8> temp{};
      alpaca::serialize(*this, temp);

      Message::Header header{
        .body_size = temp.size(),
        .message_type = Message::Type::Login,
        .opponent_id = 0,
      };

      Message message{};
      alpaca::serialize(header, message.header);
      message.body = std::move(temp);

      return message;
    }
  };

  struct RegisterPayload
  {
    std::string username;
    std::string password;
  };

  struct SendFilePayload
  {
    std::vector<u8> symmetric_key;
    std::vector<u8> files;

    [[nodiscard]] Message serialize(User::id_type id) noexcept
    {
      std::vector<u8> temp;
      alpaca::serialize(*this, temp);

      Message::Header header{
        .body_size = temp.size(),
        .message_type = Message::Type::SendFile,
        .opponent_id = id,
      };
      Message msg{};
      alpaca::serialize(header, msg.header);
      msg.body = std::move(temp);

      return msg;
    }
  };

  enum class PayloadId : u8
  {
    Login,
    Register,
    UserCheck,
    Unauthenticated,
    Authenticated,
    UserDetail,
    UserOnlineList,
  };

  static constexpr std::string_view UNAUTHENTICATED_MESSAGE{"you need to authenticate first to do this action"};
  static constexpr std::string_view AUTHENTICATED_MESSAGE{"you need to logout first to do this action"};

  template <typename T = bool>
  struct FeedbackPayload
  {
    PayloadId id;
    T response;
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
      alpaca::serialize(header, msg.header);
      msg.body = std::move(temp);
      return msg;
    }
  };
}
