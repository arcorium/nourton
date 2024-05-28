#pragma once
#include <string>
#include <vector>

#include "util/types.h"

namespace ar
{
  struct User
  {
    using id_type = u16;
    static constexpr id_type SERVER_ID = 0;

    id_type id;
    std::string name;
    std::string password;
    std::vector<u8> public_key;
  };

  struct UserResponse
  {
    User::id_type id;
    std::string name;
    // std::vector<u8> public_key;
    // DMRSA::_public_key public_key;
  };
} // namespace ar
