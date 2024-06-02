#pragma once

#include <alpaca/alpaca.h>

#include "util/types.h"

namespace ar
{
  // Feedback for specific request payload
  enum class FeedbackId : u8
  {
    Login,
    Register,
    GetUserOnline,
    GetUserDetails,
    GetServerDetails,
    StoreSymmetricKey,
    StorePublicKey,
    SendFile
  };

  static constexpr std::string_view UNAUTHENTICATED_MESSAGE{
      "you need to authenticate first to do this action"};
  static constexpr std::string_view AUTHENTICATED_MESSAGE{
      "you need to logout first to do this action"};
  static constexpr std::string_view MESSAGE_ENCRYPTION_IS_NOT_ASYMMETRIC{
      "this message should be encrypted with asymmetric one"
  };
  static constexpr std::string_view MESSAGE_ENCRYPTION_IS_NOT_SYMMETRIC{
      "the message should be encrypted with symmetric one"
  };
  static constexpr std::string_view MESSAGE_SHOULD_NOT_ENCRYPTED{
      "the message shouldn't be encrypted"
  };
  static constexpr std::string_view MESSAGE_MALFORMED{
      "the message you sent is malformed"
  };
  static constexpr std::string_view USER_NOT_FOUND{
      "you are not registered"
  };
  static constexpr std::string_view USER_ALREADY_EXIST{
      "user already exist"
  };
  static constexpr std::string_view PASSWORD_INVALID{
      "password provided is invalid"
  };

  struct FeedbackPayload
  {
    FeedbackId id;
    bool response;
    // std::optional<std::string> message;
    std::string message;

    [[nodiscard]] std::vector<u8> serialize() const noexcept
    {
      std::vector<u8> temp;
      alpaca::serialize(*this, temp);
      return temp;
    }
  };
}