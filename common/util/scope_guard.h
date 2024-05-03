#pragma once
#include <utility>

template <typename T>
  requires noexcept
(noexcept(T())) struct scope_guard
{
  T t;

  ~scope_guard() noexcept
  {
    t();
  }
};

template <typename T>
  requires noexcept
(noexcept(T())) inline static scope_guard<T> create_scope_guard(T&& t)
{
  return scope_guard<T>{std::forward<T>(t)};
}