
#include <algorithm>

#include <fmt/core.h>
#include <fmt/std.h>
#include <fmt/ranges.h>

#include <gtest/gtest.h>

#include "crypto/camellia.h"

#include "util/convert.h"

TEST(Hello, Basic)
{
  EXPECT_STREQ("Hello", "Hello");
}

template <typename T, size_t N>
static void check_array_eq(std::add_const_t<T> lhs[N], std::add_const_t<T> rhs[N]) noexcept
{
  for (size_t i = 0; i < 256; ++i)
  {
    SCOPED_TRACE(i);
    ASSERT_EQ(lhs[i], rhs[i]);
  }
}

TEST(camellia, sbox)
{
  uint8_t sboxes[3][256];
  // generate
  for (size_t i = 0; i < 256; ++i)
    sboxes[0][i] = std::rotl(ar::SBOX1[i], 1);
  for (size_t i = 0; i < 256; ++i)
    sboxes[1][i] = std::rotl(ar::SBOX1[i], 7);
  for (size_t i = 0; i < 256; ++i)
  {
    u8 j = std::rotl<u8>(i, 1);
    sboxes[2][i] = ar::SBOX1[j];
  }

  {
    SCOPED_TRACE("SBOX 2");
    check_array_eq<u8, 256>(sboxes[0], ar::SBOX2);
  }
  {
    SCOPED_TRACE("SBOX 3");
    check_array_eq<u8, 256>(sboxes[1], ar::SBOX3);
  }
  {
    SCOPED_TRACE("SBOX 4");
    check_array_eq<u8, 256>(sboxes[2], ar::SBOX4);
  }
}

TEST(camellia, camellia_block)
{
  auto a = ar::Camellia::create("mizhanaw12345jkl");
  EXPECT_EQ(a.has_value(), true);
  auto b = ar::Camellia::create("mzmzlakanabc");
  EXPECT_EQ(b.has_value(), false);

  auto& camellia = a.value();

  std::string_view text{"hello my name is"};
  auto text_span = ar::as_span(text);
  // auto text_orig = ar::to_128(text_span);

  auto cipher = camellia.encrypt(text_span);
  auto decipher = camellia.decrypt(cipher.value()).value();

  // fmt::println("Original: {}", text_span);
  // fmt::println("Decipher: {}", decipher);

  for (size_t i = 0; i < 16; ++i)
  {
    SCOPED_TRACE(i);
    ASSERT_EQ(decipher[i], text_span[i]);
  }
  // ASSERT_EQ(decipher, text_orig);
}

TEST(camellia, camellia)
{
  using namespace std::literals;

  auto a = ar::Camellia::create("mizhanaw12345jkl");
  EXPECT_EQ(a.has_value(), true);

  auto& camellia = a.value();
  auto text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque placerat."sv; // 74
  auto text_span = ar::as_span(text);
  EXPECT_EQ(text_span.size(), text.size());

  auto [garbage, cipher] = camellia.encrypts(text_span);
  EXPECT_EQ(garbage, 6);
  EXPECT_EQ(cipher.size(), 80);

  auto decipher = std::move(camellia.decrypts(cipher, garbage).value());
  EXPECT_EQ(decipher.size(), 74);

  auto decipher2 = std::move(camellia.decrypts(cipher).value());
  EXPECT_EQ(decipher2.size(), 80);

  for (size_t i = 0; i < text.size(); ++i)
  {
    SCOPED_TRACE(i);
    ASSERT_EQ(decipher[i], text_span[i]);
  }
}