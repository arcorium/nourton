#pragma once
#include <utility>
#include <atomic>

namespace ar
{
  template <std::integral T>
  class IdGenerator
  {
  public:
    IdGenerator() :
      m_val{0}
    {
    }

    IdGenerator(T val) :
      m_val{val}
    {
    }

    T gen() noexcept
    {
      return m_val.fetch_add(1);
    }

  private:
    std::atomic<T> m_val;
  };
}
