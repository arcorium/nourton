
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <fmt/std.h>
#include <gtest/gtest.h>

#include <algorithm>

#include "crypto/camellia.h"
#include "util.h"
#include "util/convert.h"
#include "util/file.h"
#include "util/algorithm.h"

TEST(camellia, sbox)
{
  uint8_t sboxes[3][256];
  // generate
  for (size_t i = 0; i < 256; ++i)
    sboxes[0][i] = std::rotl(ar::SBOX1[i], 1);
  for (size_t i = 0; i < 256; ++i)
    sboxes[1][i] = std::rotl(ar::SBOX1[i], 7);
  for (size_t i = 0; i < 256; ++i) {
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

  auto &camellia = a.value();

  std::string_view text{"hello my name is"};
  auto text_span = ar::as_span(text);
  // auto text_orig = ar::to_128(text_span);

  auto cipher = camellia.encrypt(text_span);
  auto decipher = camellia.decrypt(cipher.value()).value();

  for (size_t i = 0; i < 16; ++i) {
    SCOPED_TRACE(i);
    ASSERT_EQ(decipher[i], text_span[i]);
  }
  // ASSERT_EQ(decipher, text_orig);
}

TEST(camellia, camellia)
{
  auto a = ar::Camellia::create("mizhanaw12345jkl");
  EXPECT_EQ(a.has_value(), true);

  auto keys = a->key();
  auto orig_spans = ar::as_span("mizhanaw12345jkl"sv);

  check_span_eq<const u8>(keys, orig_spans);

  auto &camellia = a.value();
  auto text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque placerat."sv;  // 74
  auto text_span = ar::as_span(text);
  EXPECT_EQ(text_span.size(), text.size());

  auto [garbage, cipher] = camellia.encrypts(text_span);
  EXPECT_EQ(garbage, 6);
  EXPECT_EQ(cipher.size(), 80);

  auto decipher = std::move(camellia.decrypts(cipher, garbage).value());
  EXPECT_EQ(decipher.size(), 74);

  auto decipher2 = std::move(camellia.decrypts(cipher).value());
  EXPECT_EQ(decipher2.size(), 80);

  check_span_eq<u8>(decipher, text_span);
}

TEST(camellia, random_generated_key)
{
  auto key = ar::random_bytes<ar::KEY_BYTE>();
  auto a = ar::Camellia{ar::Camellia::key_type{key}};

  auto &camellia = a;
  auto text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque placerat."sv;  // 74
  auto text_span = ar::as_span(text);
  EXPECT_EQ(text_span.size(), text.size());

  auto [garbage, cipher] = camellia.encrypts(text_span);
  EXPECT_EQ(garbage, 6);
  EXPECT_EQ(cipher.size(), 80);

  auto decipher = std::move(camellia.decrypts(cipher, garbage).value());
  EXPECT_EQ(decipher.size(), 74);

  auto decipher2 = std::move(camellia.decrypts(cipher).value());
  EXPECT_EQ(decipher2.size(), 80);

  check_span_eq<u8>(decipher, text_span);
}

TEST(camellia, unconstructed)
{
  auto a = ar::Camellia{};
  auto data = ar::random_bytes<16>();
  auto enc_result = a.encrypt(data);
  ASSERT_FALSE(enc_result.has_value());

  auto dec_result = a.decrypt(data);
  ASSERT_FALSE(dec_result.has_value());
}

TEST(camellia, moved_object)
{
  auto a = ar::Camellia{};
  auto data = ar::random_bytes<16>();
  auto enc_result = a.encrypt(data);
  ASSERT_FALSE(enc_result.has_value());

  auto keys = ar::random_bytes<ar::KEY_BYTE>();
  a = ar::Camellia{keys};
  enc_result = a.encrypt(data);
  ASSERT_TRUE(enc_result.has_value());
}

TEST(camellia, encrypt_file)
{
  auto plain_result = ar::read_file_as_bytes("../../resource/image/docs.png");
  ASSERT_TRUE(plain_result.has_value());

  for (usize i = 0; i < 100; ++i) {
    ar::Camellia camellia{};

    auto [filler, cipher] = camellia.encrypts(plain_result.value());
    auto decipher = camellia.decrypts(cipher, filler);
    ASSERT_TRUE(decipher.has_value());

    check_span_eq<u8, u8>(plain_result.value(), decipher.value());
  }
}
