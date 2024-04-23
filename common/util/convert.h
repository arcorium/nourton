#pragma once

#include <expected>
#include <ranges>
#include <span>

#include "types.h"

namespace ar
{
	static u128 to_128(const std::span<const u8, 128 / 8> key) noexcept
	{
		u128 temp{};
		constexpr usize n = 128 / 8;

		for (usize i = 0; i < n; ++i)
			temp = (temp << 8) | key[i];
		return temp;
	}

	template <unsigned Bits, boost::multiprecision::cpp_integer_type SignType,
	          boost::multiprecision::cpp_int_check_type Checked>
	boost::multiprecision::number<
		boost::multiprecision::cpp_int_backend<Bits, Bits, SignType, Checked, void>>
	static rawToBoost(const void* V)
	{
#if BOOST_ENDIAN_BIG_BYTE
		static const auto msv_first = true;
#else
		static const auto msv_first = false;
#endif

		boost::multiprecision::number<boost::multiprecision::cpp_int_backend<Bits, Bits, SignType, Checked, void>> ret;
		auto VPtr = reinterpret_cast<const unsigned char*>(V);
		boost::multiprecision::import_bits(ret, VPtr, VPtr + (Bits / 8), 0, msv_first);
		return ret;
	}

	static boost::multiprecision::int128_t rawToBoost_int128(const void* V)
	{
		using namespace boost::multiprecision;
		return rawToBoost<128, signed_magnitude, unchecked>(V);
	}

	static boost::multiprecision::int128_t rawToBoost_int128_safe(const void* V)
	{
		using namespace boost::multiprecision;
		return rawToBoost<128, signed_magnitude, checked>(V);
	}

	static boost::multiprecision::uint128_t rawToBoost_uint128(const void* V)
	{
		using namespace boost::multiprecision;
		return rawToBoost<128, unsigned_magnitude, unchecked>(V);
	}

	static boost::multiprecision::uint128_t rawToBoost_uint128_safe(const void* V)
	{
		using namespace boost::multiprecision;
		return rawToBoost<128, unsigned_magnitude, checked>(V);
	}

	inline u128 rotl(const u128& val, int rotation) noexcept
	{
		return (val << rotation | val >> 128 - rotation);
	}

	template <typename To, typename From> requires requires { sizeof(To) == 2 * sizeof(From); }
	To combine(From lsb, From msb) noexcept
	{
		return (static_cast<To>(msb) << (sizeof(From) * 8)) | lsb;
	}

	inline u128 combine(u64 lsb, u64 msb) noexcept
	{
		u128 result = msb;
		return (result << 64) | lsb;
	}


	template <typename T, typename U>
	constexpr std::span<T, sizeof(U) / sizeof(T)> as_span(U& data_) noexcept
	{
		return std::span<T, sizeof(U) / sizeof(T)>{reinterpret_cast<T*>(&data_), sizeof(U) / sizeof(T)};
	}

	template <typename T>
	constexpr std::span<u8> as_bytes(std::span<T> val, usize end_offset = 0) noexcept
	{
		return std::span<u8>{(u8*)val.data(), val.size_bytes() - end_offset};
	}

	constexpr std::span<u8> as_span(std::string_view val) noexcept
	{
		return std::span<u8>{(u8*)val.data(), val.size()};
	}

	constexpr std::array<u8, 16> combine_to_bytes(u64 lsb, u64 msb) noexcept
	{
		std::array<u8, 16> result{};

		auto lsb_span = as_span<const u8>(lsb);
		auto msb_span = as_span<const u8>(msb);

		for (const auto& [i, byte] : msb_span | std::views::enumerate)
			result[i] = byte;
		for (const auto& [i, byte] : lsb_span | std::views::enumerate)
			result[sizeof(u64) + i] = byte;

		return result;
	}
}
