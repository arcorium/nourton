#pragma once
#include <expected>

namespace ar
{
  /**
   * helper to make expected value and prevent move constructor
   * @tparam T expected type
   * @tparam E error type
   * @tparam Arg constructor types
   * @param args constructor arguments
   * @return std::expected with T value
   */
  template <typename T, typename E, typename... Arg>
  std::expected<T, E> make_expected(
    Arg&&... args) noexcept(noexcept(std::expected<T, E>{std::in_place, std::forward<Arg>(args)...}))
  {
    return std::expected<T, E>{std::in_place, std::forward<Arg>(args)...};
  }

  /**
   * helper to make unexpected value and prevent double move constructor
   * @tparam E error type
   * @tparam Arg constructor types
   * @param args constructor arguments
   * @return std::unexpected with E value
   */
  template <typename E, typename... Arg>
  std::unexpected<E> unexpected(Arg&&... args) noexcept(noexcept(std::unexpected<E>{
    std::in_place, std::forward<Arg>(args)...
  }))
  {
    return std::unexpected<E>{std::in_place, std::forward<Arg>(args)...};
  }

  /**
   * helper to make optional value and prevent move constructor
   * @tparam T optional type
   * @tparam Arg constructor types
   * @param args constructor arguments
   * @return std::optional with T value
   */
  template <typename T, typename... Arg>
  std::optional<T> make_optional(
    Arg&&... args) noexcept(noexcept(std::optional<T>{std::in_place, std::forward<Arg>(args)...}))
  {
    return std::optional<T>{std::in_place, std::forward<Arg>(args)...};
  }
}
