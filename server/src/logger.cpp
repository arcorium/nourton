//
// Created by mizzh on 4/15/2024.
//

#include "logger.h"

#include <iostream>
#include <fmt/core.h>
#include <fmt/std.h>
#include <fmt/chrono.h>
#include <thread>

namespace ar
{
  void Logger::trace(std::string_view val, std::source_location sl) noexcept
  {
    log(HEADER_NAME, TRACE_HEADER, val, sl);
  }

  void Logger::info(std::string_view val, std::source_location sl) noexcept
  {
    log(HEADER_NAME, INFO_HEADER, val, sl);
  }

  void Logger::warn(std::string_view val, std::source_location sl) noexcept
  {
    log(HEADER_NAME, WARN_HEADER, val, sl);
  }

  void Logger::critical(std::string_view val, std::source_location sl) noexcept
  {
    log(HEADER_NAME, CRITICAL_HEADER, val, sl);
  }

  void Logger::log(std::string_view header, std::string_view type, std::string_view val, const std::source_location &sl)
  {
#ifdef _DEBUG
    fmt::println("[{}] {} | {} | '{}' => {}", std::this_thread::get_id(), std::chrono::system_clock::now(), type,
                 sl.function_name(), val);
#endif
  }
} // namespace ar
