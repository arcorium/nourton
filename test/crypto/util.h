#pragma once

#include <span>

#include <gtest/gtest.h>
#include <span>

using namespace std::literals;

template <typename T, size_t N>
static void check_array_eq(std::add_const_t<T> lhs[N], std::add_const_t<T> rhs[N]) noexcept
{
  for (size_t i = 0; i < N; ++i)
  {
    SCOPED_TRACE(i);
    ASSERT_EQ(lhs[i], rhs[i]);
  }
}

template <typename T, typename U>
static void check_span_eq(std::span<T> lhs, std::span<U> rhs) noexcept
{
  ASSERT_EQ(lhs.size(), rhs.size());

  for (size_t i = 0; i < lhs.size(); ++i)
  {
    SCOPED_TRACE(i);
    EXPECT_EQ(lhs[i], rhs[i]);
  }
}

template <typename T, typename U>
static void check_span_ne(std::span<T> lhs, std::span<U> rhs) noexcept
{
  ASSERT_EQ(lhs.size(), rhs.size());

  for (size_t i = 0; i < lhs.size(); ++i)
  {
    SCOPED_TRACE(i);
    ASSERT_NE(lhs[i], rhs[i]);
  }
}
