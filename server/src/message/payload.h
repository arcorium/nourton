#pragma once
#include "../util/make.h"
#include "../util/types.h"

#include <array>
#include <expected>
#include <vector>
#include <span>

#include <alpaca/alpaca.h>

#include "../server.h"
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

    constexpr static u8 size = sizeof(Header);
    std::array<u8, size> header; //TODO: Combine header and body, so it doesn't need to combine it later
    std::vector<u8> body;

    [[nodiscard]]
    Header parse_header() const noexcept
    {
      Header temp{};
      std::memcpy(&temp, header.data(), size);
      return temp;
    }


    [[nodiscard]] Header const* get_header() const noexcept
    {
      return reinterpret_cast<Header const*>(header.data());
    }

    template <typename T>
    [[nodiscard]] T body_as() const noexcept
    {
      // TODO: Implement it
      return T{};
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
  };

  struct FeedbackPayload
  {
    PayloadId id;
    bool response;
    std::optional<std::string> message;

    [[nodiscard]] Message serialize() const noexcept
    {
      std::vector<u8> temp;
      alpaca::serialize(*this, temp);

      auto header = Message::Header{
        .body_size = temp.size(),
        .message_type = Message::Type::Feedback,
        .opponent_id = 0
      };

      auto msg = Message{};
      alpaca::serialize(header, msg.header);
      msg.body = std::move(temp);
      return msg;
    }
  };
}
