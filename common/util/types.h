#pragma once

#include <boost/multiprecision/integer.hpp>
#include <boost/multiprecision/number.hpp>
#include <cstdint>
#include <utility>

using u8 = uint8_t;
using byte = u8;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using usize = size_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using isize = intmax_t;

using f32 = float;
using f64 = double;

static_assert((sizeof(u8) & sizeof(byte) & sizeof(i8)) == 1);
static_assert((sizeof(u16) & sizeof(i16)) == 2);
static_assert((sizeof(u32) & sizeof(i32)) == 4);
static_assert((sizeof(u64) & sizeof(i64)) == 8);
static_assert((sizeof(usize) & sizeof(isize)) == sizeof(u64));

static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);

// extras
using u128 = boost::multiprecision::uint128_t;
using u256 = boost::multiprecision::uint256_t;
using u512 = boost::multiprecision::uint512_t;
using i128 = boost::multiprecision::int128_t;
using i256 = boost::multiprecision::int256_t;
using i512 = boost::multiprecision::int512_t;

// static_assert(sizeof(u128) & sizeof(i128));

// concept
namespace ar
{
  template <typename T>
  concept number = requires(T t) { std::integral<T> || std::floating_point<T>; };

  template <usize Bytes>
  struct uint;

  template <>
  struct uint<1>
  {
    using type = u8;
  };

  template <>
  struct uint<2>
  {
    using type = u16;
  };

  template <>
  struct uint<4>
  {
    using type = u32;
  };

  template <>
  struct uint<8>
  {
    using type = u64;
  };

  template <usize Byte>
  struct sint;

  template <>
  struct sint<1>
  {
    using type = i8;
  };

  template <>
  struct sint<2>
  {
    using type = i16;
  };

  template <>
  struct sint<4>
  {
    using type = i32;
  };

  template <>
  struct sint<8>
  {
    using type = i64;
  };

  // Extras

  template <>
  struct sint<16>
  {
    using type = i128;
  };

  template <>
  struct uint<16>
  {
    using type = u128;
  };

  template <>
  struct sint<32>
  {
    using type = i256;
  };

  template <>
  struct uint<32>
  {
    using type = u256;
  };

  template <>
  struct sint<64>
  {
    using type = i512;
  };

  template <>
  struct uint<64>
  {
    using type = u512;
  };

  template <typename T>
  consteval usize size_of()
  {
    if constexpr (std::is_same_v<T, u128> || std::is_same_v<T, i128>)
      return 128 / 8;
    if constexpr (std::is_same_v<T, u256> || std::is_same_v<T, i256>)
      return 256 / 8;
    if constexpr (std::is_same_v<T, u512> || std::is_same_v<T, i512>)
      return 512 / 8;

    return sizeof(T);
  }

  template <typename T>  // WARN: T for t will return the T itself
  using signed_type_of = typename sint<size_of<T>()>::type;

  template <typename T>
  using unsigned_type_of = typename uint<size_of<T>()>::type;

  // Check

  static_assert(std::is_same_v<signed_type_of<u64>, i64>);
  static_assert(std::is_same_v<signed_type_of<i32>, i32>);
  static_assert(std::is_same_v<unsigned_type_of<i64>, u64>);
  static_assert(std::is_same_v<unsigned_type_of<u32>, u32>);

  // Extras
  static_assert(std::is_same_v<signed_type_of<u128>, i128>);
  static_assert(std::is_same_v<unsigned_type_of<i128>, u128>);
}  // namespace ar
