#pragma once

#include <atomic>
#include <utility>

namespace ar
{
  template <std::integral T>
  class IdGenerator
  {
  public:
    IdGenerator()
        : val_{0}
    {
    }

    IdGenerator(T val)
        : val_{val}
    {
    }

    T gen()

        noexcept
    {
      return val_.fetch_add(1);
    }

  private:
    std::atomic<T> val_;
  };
}  // namespace ar
