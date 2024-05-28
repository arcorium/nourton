#pragma once

#include <alpaca/alpaca.h>

#include "util/types.h"

namespace ar
{
  // Feedback for specific request payload
  enum class PayloadId : u8
  {
    Login,
    Register,
    GetUserOnline,
    GetUserDetails,
    StoreSymmetricKey,
    StorePublicKey,
    SendFile
  };

  static constexpr std::string_view UNAUTHENTICATED_MESSAGE{
      "you need to authenticate first to do this action"};
  static constexpr std::string_view AUTHENTICATED_MESSAGE{
      "you need to logout first to do this action"};

  struct FeedbackPayload
  {
    PayloadId id;
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