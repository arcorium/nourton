#pragma once
#include <expected>
#include <vector>
#include <fstream>
#include <span>
#include <filesystem>

#include "make.h"
#include "types.h"

#include <fmt/format.h>

namespace ar
{
	static std::expected<std::vector<u8>, std::string_view> read_file_as_bytes(
		std::string_view filepath) noexcept
	{
		using namespace std::literals;

		std::ifstream file{filepath.data(), std::ios::binary};
		if (!file.is_open())
			return std::unexpected("failed to open file"sv);

		// std::vector<u8> buffer{std::istreambuf_iterator<char>{file}, {}};

		// PERF: Using std::fstream::read is much much faster than using iterator
		std::vector<u8> buffer{};
		file.seekg(0, std::ifstream::end);
		buffer.resize(file.tellg());
		file.seekg(0, std::ifstream::beg);
		file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

		file.close();
		return ar::make_expected<std::vector<u8>, std::string_view>(std::move(buffer));
	}

	template <bool Overwrite = false>
	static bool save_bytes_as_file(std::string_view dest_path, std::span<u8> bytes) noexcept
	{
		// Prevent overwrite file
		if constexpr (!Overwrite)
		{
			std::error_code ec;
			if (std::filesystem::exists(dest_path, ec))
				return false;
		}

		std::ofstream file{dest_path.data(), std::ios::binary};
		if (!file.is_open())
			return false;
		file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
		file.close();
		return true;
	}

	static bool delete_file(std::string_view fullpath) noexcept
	{
		std::error_code ec;
		std::filesystem::remove_all(fullpath, ec);
		return !ec;
	}

	template <bool Block = true>
	static void execute_file(std::string_view fullpath) noexcept
	{
		auto str = fmt::format("\"{}\"", fullpath);
		auto res = std::system(str.data());
	}
}
