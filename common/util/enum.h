#pragma once

#include <utility>

namespace ar
{
  template <typename T> requires std::is_scoped_enum_v<T>
  static constexpr std::underlying_type_t<T> to_underlying(T val) noexcept
  {
    return static_cast<std::underlying_type_t<T>>(val);
  }
}
