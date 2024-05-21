#include "crypto/aes.h"

#include <fmt/core.h>
#include <fmt/ranges.h>
#include <gtest/gtest.h>

#include <array>

#include "aes/AES.h"
#include "util.h"
#include "util/algorithm.h"
#include "util/file.h"
#include "util/types.h"

TEST(aes, wrapper_encrypt_block)
{
  ar::AES aes{};

  auto plain = ar::random_bytes<16>();
  auto cipher = aes.encrypt(plain);
  ASSERT_TRUE(cipher.has_value());

  auto decipher = aes.decrypt(cipher.value());
  ASSERT_TRUE(decipher.has_value());

  check_span_eq<u8, u8>(plain, decipher.value());
}

TEST(aes, wrapper_encypt_block_with_remaining_bytes)
{
  ar::AES aes{};

  auto plain = ar::random_bytes<18>();
  auto cipher = aes.encrypt(plain);
  ASSERT_FALSE(cipher.has_value());
}

TEST(aes, wrapper_encrypt_arbitrary_bytes)
{
  ar::AES aes{};

  auto plain = ar::random_bytes<18>();
  auto [filler, cipher] = aes.encrypts(plain);

  ASSERT_EQ(filler, ar::AES::KEY_BYTE - 2);

  auto decipher = aes.decrypts(cipher, filler);
  ASSERT_TRUE(decipher.has_value());

  check_span_eq<u8, u8>(decipher.value(), plain);
}

TEST(aes, wrapper_encrypt_file)
{
  auto plain_result = ar::read_file_as_bytes("../../resource/image/docs.png");
  ASSERT_TRUE(plain_result.has_value());

  for (usize i = 0; i < 100; ++i)
  {
    ar::AES aes{};

    ASSERT_TRUE(plain_result.has_value());

    auto [filler, cipher] = aes.encrypts(plain_result.value());

    auto decipher = aes.decrypts(cipher, filler);
    ASSERT_TRUE(decipher.has_value());

    check_span_eq<u8, u8>(decipher.value(), plain_result.value());
  }
}
