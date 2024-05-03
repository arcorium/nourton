#pragma once
#include <expected>
#include <primesieve.hpp>
#include <random>
#include <string_view>

#include "types.h"

namespace ar
{
  // @copyright https://www.geeksforgeeks.org/gcd-in-cpp/
  // template <ar::number T>
  template <typename T>  // HACK: use generic to allow boost::multiprecision
  static constexpr T gcd(T a, T b) noexcept
  {
    if (!a)
      return b;
    if (!b)
      return a;

    while (true)
    {
      if (a == b)
        return b;

      if (a > b)
        a -= b;
      else
        b -= a;
    }
    std::unreachable();
  }

  // @copyright: https://www.geeksforgeeks.org/multiplicative-inverse-under-modulo-m/
  template <typename T>
  static constexpr T gcd_extended(T a, T b, signed_type_of<T>& x, signed_type_of<T>& y)
  {
    // Base Case
    if (!a)
    {
      x = 0;
      y = 1;
      return b;
    }

    signed_type_of<T> x1, y1;  // To store results of recursive call
    T gcd = gcd_extended(b % a, a, x1, y1);

    // Update x and y using results of recursive call
    x = y1 - x1 * (b / a);
    y = x1;

    return gcd;
  }

  // @copyright: https://www.geeksforgeeks.org/multiplicative-inverse-under-modulo-m/
  template <typename T>
  static constexpr std::expected<T, std::string_view> mod_inverse(T a, T m)
  {
    // search y on a * y = 1 mod m
    // a * y mod m = 1
    using signed_t = ar::signed_type_of<T>;

    signed_t x, y;
    // ax + my = gcd(a, m)
    // ax + my = 1
    // ax = 1 mod m
    T g = gcd_extended(a, m, x, y);
    if (g != 1)
      return std::unexpected("GCD of a and m should be 1");
    // return std::abs(x % m) % m; // std::abs couldn't handle boost::multiprecision numbers

    signed_t res;
    if constexpr (size_of<T>() > size_of<u64>())
      res = x % m.template convert_to<signed_t>();  // handle conversion for boost::multiprecision
    else
      res = x % static_cast<signed_t>(m);

    // res += m;
    return T{(res + m) % m};
  }

  template <typename T>
  static constexpr T random(T min = std::numeric_limits<T>::min(),
                            T max = std::numeric_limits<T>::max()) noexcept
  {
    static std::random_device random{};
    std::mt19937 engine{random()};

    std::uniform_int_distribution<T> dis{min, max};
    return dis(engine);
  }

  template <usize N>
  static constexpr std::array<u8, N> random_bytes() noexcept
  {
    static std::random_device random{};
    std::mt19937 engine{random()};

    // PERF: generate uint<N>_t and split it by bytes
    std::uniform_int_distribution<> dis{0, std::numeric_limits<u8>::max()};
    std::array<u8, N> result{};
    std::generate_n(result.begin(), N, std::bind(dis, engine));
    return result;
  }

#ifdef AR_USE_PRIMESIEVE
  #define USE_NAIVE_PRIME 0
#else
  #define USE_NAIVE_PRIME 1
#endif

  template <typename T>
  static constexpr bool is_prime(T n)
  {
    if (n <= 1)
      return false;  // 0 and 1 is not prime
    if (n % 2 == 0)
      return n == 2;  // all numbers divided by 2 except 2 is not prime
    if (n % 3 == 0)
      return n == 3;  // all numbers divided by 3 except 3 is not prime

    // PERF: add stepping
    T mid = n / 2;
    for (T i = 5; i < mid; ++i)
    {
      if (n % i == 0)
        return false;
    }
    return true;
  }

  template <typename T>
  static constexpr T nth_prime(usize nth) noexcept
  {
    // NOTE: Naive implementation
    if constexpr (USE_NAIVE_PRIME)
    {
      usize counter{1};
      T number{2};
      while (counter < nth)
      {
        if (is_prime(number))
          ++counter;
        ++number;
      }
      return number - 1;
    }

    return primesieve::nth_prime(static_cast<int64_t>(nth));
  }

#ifdef AR_USE_BOOST_POWM
  #define USE_NAIVE_POWM 0
#else
  #define USE_NAIVE_POWM 1
#endif

  // a ^ b % m
  // ((a % m) ( b % m )) % m
  // @copyright https://www.geeksforgeeks.org/modular-exponentiation-power-in-modular-arithmetic/
  template <typename T>
  static constexpr T mod_exponential(T a, T b, T m) noexcept
  {
    if constexpr (USE_NAIVE_POWM)
    {
      T res = 1;  // Initialize result

      a = a % m;  // Update x if it is more than or

      if (a == 0)
        return 0;  // In case x is divisible by p;

      while (b > 0)
      {
        // If y is odd, multiply x with result
        if (b & 1)
          res = (res * a) % m;

        b >>= 1;  // divide by 2^1
        a = (a * a) % m;
      }
      return res;
    }
    return boost::multiprecision::powm<T>(a, b, m);
  }

  template <typename T>
  static constexpr T phi(T n) noexcept
  {
    T result = 1;
    for (T i = 2; i < n; ++i)
      if (gcd(i, n) == 1)
        ++result;
    return result;
  }
}  // namespace ar
