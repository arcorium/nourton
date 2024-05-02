#pragma once

#include <vector>
#include <string_view>

#include "types.h"

namespace ar
{
	static std::vector<std::string_view> split_filepath(const char* multiple_path) noexcept
	{
		std::vector<std::string_view> result;
		std::string_view paths{multiple_path};

		usize first_idx = 0;
		while (true)
		{
			auto last_idx = paths.find_first_of('|', first_idx);
			if (last_idx == std::string_view::npos)
				break;
			auto sub = paths.substr(first_idx, last_idx - first_idx);

			result.emplace_back(sub);
			first_idx = last_idx + 1;
		}

		return result;
	}
}
