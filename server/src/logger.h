#pragma once
#include <source_location>
#include <string_view>

namespace ar
{
  using namespace std::literals;

  class Logger
  {
  public:
    static void trace(std::string_view val, std::source_location sl = std::source_location::current()) noexcept;
    static void info(std::string_view val, std::source_location sl = std::source_location::current()) noexcept;
    static void warn(std::string_view val, std::source_location sl = std::source_location::current()) noexcept;
    static void critical(std::string_view val, std::source_location sl = std::source_location::current()) noexcept;

  private:
    static void log(std::string_view header, std::string_view type, std::string_view val,
                    const std::source_location& sl);

  private:
    static constexpr std::string_view HEADER_NAME{"nourton"sv};
    static constexpr std::string_view TRACE_HEADER{"TRACE"sv};
    static constexpr std::string_view INFO_HEADER{"INFO"sv};
    static constexpr std::string_view WARN_HEADER{"WARN"sv};
    static constexpr std::string_view CRITICAL_HEADER{"CRITICAL"sv};
  };
}
