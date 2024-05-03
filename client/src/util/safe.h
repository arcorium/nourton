#pragma once

#include <util/concept.h>

#include <mutex>
#include <optional>

namespace ar
{
  template <typename T, typename Mtx = std::mutex>
  class safe_object
  {
    safe_object() = default;

    //    template<typename Lock = std::unique_lock<Mtx>>
    //    Lock lock() noexcept;

    //    template<typename Lock = std::unique_lock<Mtx>>
    //    std::optional<Lock> try_lock() noexcept;

   public:
    Mtx mutex_;
    T object_;
  };
}  // namespace ar
