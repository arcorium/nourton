#pragma once
#include <string>
#include <vector>

#include "util/types.h"

namespace ar
{
  struct User
  {
    using id_type = u16;

    id_type id;
    std::string name;
    std::string password;
    std::vector<u8> public_key;
  };
}
