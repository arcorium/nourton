#pragma once

#include <fmt/chrono.h>

#include <chrono>
#include <string>

namespace ar
{
  inline std::string get_current_time() noexcept
  {
    auto current_time = std::chrono::system_clock::now();
    return fmt::format("{:%Y-%m-%d %H:%M}", current_time);
  }
}  // namespace ar
