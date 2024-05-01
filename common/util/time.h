#pragma once

#include <string>
#include <chrono>

#include <fmt/chrono.h>

namespace ar
{
	inline std::string get_current_time() noexcept
	{
		auto current_time = std::chrono::system_clock::now();
		return fmt::format("{:%Y-%m-%d %H:%M}", current_time);
	}
}
