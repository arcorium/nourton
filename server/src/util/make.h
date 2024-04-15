#pragma once
#include <expected>

namespace ar {
template <typename T, typename E, typename... Arg>
std::expected<T, E> make_expected(
    Arg &&... args) {
  return std::expected<T, E>{std::in_place, args...};
}
}