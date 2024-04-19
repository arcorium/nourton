#include "logger.h"

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/std.h>
#include <fmt/color.h>

#include <thread>

#include "util/types.h"

namespace ar
{
  std::string_view get_function_name(std::string_view func_) noexcept
  {
    auto idx = func_.find("__cdecl");
    if (idx == std::string_view::npos)
      return "error"sv;

    usize len = func_.size() - (idx + 8);

    auto idx2 = func_.find_last_of('(');
    if (idx2 == std::string_view::npos)
      return "error"sv;

    len -= func_.size() - idx2;
    return func_.substr(idx + 8, len);
  }

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

  void Logger::log(std::string_view header, std::string_view type,
                   std::string_view val, const std::source_location& sl)
  {
    if constexpr (_DEBUG)
      fmt::println("[{:^7}] {} |{:^8}| '{}' => {}",
                   std::this_thread::get_id(),
                   std::chrono::system_clock::now(), type,
                   get_function_name(sl.function_name()), val);
  }
} // namespace ar
