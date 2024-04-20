#pragma once
#include <map>
#include <source_location>
#include <string_view>
#include <thread>

namespace ar
{
  using namespace std::literals;

  class Logger
  {
    using thread_id = std::thread::id;

  public:
    enum class Level
    {
      Trace,
      Info,
      Warn,
      Critical
    };

    static void trace(std::string_view val, std::source_location sl = std::source_location::current()) noexcept;
    static void info(std::string_view val, std::source_location sl = std::source_location::current()) noexcept;
    static void warn(std::string_view val, std::source_location sl = std::source_location::current()) noexcept;
    static void critical(std::string_view val, std::source_location sl = std::source_location::current()) noexcept;

    static void set_minimum_level(Level level) noexcept;
    static void set_thread_name(std::string name, thread_id id) noexcept;
    static void set_current_thread_name(std::string name) noexcept;

  private:
    static void log(Level level, std::string_view type, std::string_view val,
                    const std::source_location& sl);

  private:
    static inline std::map<thread_id, std::string> s_thread_names;
    static inline Level s_level{Level::Trace};

    static constexpr std::string_view HEADER_NAME{"nourton"sv}; // NOTE: Currently not used
    static constexpr std::string_view TRACE_HEADER{"TRACE"sv};
    static constexpr std::string_view INFO_HEADER{"INFO"sv};
    static constexpr std::string_view WARN_HEADER{"WARN"sv};
    static constexpr std::string_view CRITICAL_HEADER{"CRITICAL"sv};
  };
}
