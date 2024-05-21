#pragma once
#include <mutex>

namespace ar
{
  template <typename T>
  concept lockable = requires(T t) {
    { t.lock() } -> std::same_as<void>;
    { t.unlock() } -> std::same_as<void>;
  };

  template <typename T>
  concept basic_lockable = requires(T t) {
    lockable;
    { t.try_lock() } -> std::same_as<bool>;
  };
}  // namespace ar
