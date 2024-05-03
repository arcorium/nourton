#include "logger.h"

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/std.h>

#include <thread>

#include "core.h"
#include "util/types.h"

namespace ar
{
  static constexpr std::string_view get_function_name(std::string_view func_) noexcept
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
    log(Level::Trace, TRACE_HEADER, val, sl);
  }

  void Logger::info(std::string_view val, std::source_location sl) noexcept
  {
    log(Level::Info, INFO_HEADER, val, sl);
  }

  void Logger::warn(std::string_view val, std::source_location sl) noexcept
  {
    log(Level::Warn, WARN_HEADER, val, sl);
  }

  void Logger::error(std::string_view val, std::source_location sl) noexcept
  {
    log(Level::Error, ERROR_HEADER, val, sl);
  }

  void Logger::critical(std::string_view val, std::source_location sl) noexcept
  {
    log(Level::Critical, CRITICAL_HEADER, val, sl);
    std::abort();
  }

  void Logger::set_minimum_level(Level level) noexcept
  {
    s_level = level;
  }

  void Logger::set_thread_name(std::string name, thread_id id) noexcept
  {
    s_thread_names[id] = std::move(name);
  }

  void Logger::set_current_thread_name(std::string name) noexcept
  {
    set_thread_name(std::move(name), std::this_thread::get_id());
  }

  void Logger::log(Level level, std::string_view type, std::string_view val,
                   const std::source_location& sl)
  {
    if (level < s_level)
      return;

    auto id = std::this_thread::get_id();
    auto thread = fmt::format("{}", id);
    if (s_thread_names.contains(id))
      thread = s_thread_names[id];

    fmt::println("[{:^8}] {} |{:^8}| '{}' => {}", thread, std::chrono::system_clock::now(), type,
                 get_function_name(sl.function_name()), val);
  }
}  // namespace ar
