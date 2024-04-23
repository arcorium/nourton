#include <fmt/ostream.h>
#include <fmt/ranges.h>

#include <gtest/gtest.h>

#include "util.h"

#include "crypto/dm_rsa.h"

#include "util/convert.h"

TEST(rsa, prime_key_generated)
{
  ar::RSA rsa{};
  u16 message = 0xDEED;
  auto cipher = rsa.encrypt(message);
  ASSERT_TRUE(cipher.has_value());
  auto decipher = rsa.decrypt(cipher.value());
  EXPECT_EQ(message, decipher);

  u16 bigger_message = std::numeric_limits<u16>::max();
  cipher = rsa.encrypt(bigger_message);
  ASSERT_TRUE(cipher.has_value());
  decipher = rsa.decrypt(cipher.value());
  EXPECT_EQ(decipher, bigger_message);
}

TEST(rsa, prime_key_inputted)
{
  ar::RSA rsa{11, 17};
  u8 message = 0xD;
  auto cipher = rsa.encrypt(message);
  ASSERT_TRUE(cipher.has_value());
  auto decipher = rsa.decrypt(cipher.value());

  ASSERT_EQ(message, decipher);

  u16 bigger_message = 0xDEED;
  cipher = rsa.encrypt(bigger_message);
  ASSERT_FALSE(cipher.has_value());
}

TEST(rsa, encrypt_bytes)
{
  std::string_view message{"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque placerat."};
  auto message_bytes = ar::as_span(message);
  usize expected_filler = 0;
  if (auto result = message_bytes.size() % ar::size_of<ar::RSA::block_type>())
    expected_filler = ar::size_of<ar::RSA::block_type>() - result;

  ar::RSA rsa{};
  auto [filler, cipher] = rsa.encrypts(message_bytes);
  EXPECT_EQ(cipher.size(), message_bytes.size() / ar::size_of<ar::RSA::block_type>() + (expected_filler ? 1 : 0));
  EXPECT_EQ(filler, expected_filler);
  auto cipher_bytes = ar::as_bytes<ar::RSA::block_enc_type>(cipher);
  EXPECT_EQ(cipher_bytes.size(),
            std::ceil(static_cast<float>(message_bytes.size()) / ar::size_of<ar::RSA::block_type>()) * ar::size_of<ar::
            RSA::block_enc_type>(
            ));

  auto decipher = rsa.decrypts(cipher_bytes);
  ASSERT_TRUE(decipher.has_value());
  // auto& decipher_bytes = decipher.value();
  auto decipher_bytes = ar::as_bytes<ar::RSA::block_type>(decipher.value(), filler);

  check_span_eq<u8, u8>(message_bytes, decipher_bytes);
}

TEST(dm_rsa, prime_key_inputted)
{
  ar::DMRSA rsa{11, 29, 13, 23};
  u8 message = 0xDE;
  auto cipher = rsa.encrypt(message);
  ASSERT_TRUE(cipher.has_value());

  auto decipher = rsa.decrypt(cipher.value());
  ASSERT_EQ(decipher, message);

  u32 bigger_message = 0xDEEDBEEF;
  cipher = rsa.encrypt(bigger_message);
  ASSERT_FALSE(cipher.has_value());
}

TEST(dm_rsa, prime_key_generated)
{
  ar::DMRSA rsa{};
  u32 message = 0xDEEDBEEF;
  auto cipher = rsa.encrypt(message);
  ASSERT_TRUE(cipher.has_value());

  auto decipher = rsa.decrypt(cipher.value());
  ASSERT_EQ(decipher, message);
}

TEST(dm_rsa, encrypt_bytes)
{
  std::string_view message{"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque placerat."};
  auto message_bytes = ar::as_span(message);
  usize expected_filler = 0;
  if (auto result = message.size() % ar::size_of<ar::DMRSA::block_type>())
    expected_filler = ar::size_of<ar::DMRSA::block_type>() - result;

  ar::DMRSA rsa{};
  auto [filler, cipher] = rsa.encrypts(message_bytes);
  EXPECT_EQ(cipher.size(), message_bytes.size() / ar::size_of<ar::DMRSA::block_type>() + (expected_filler ? 1 : 0));
  EXPECT_EQ(filler, expected_filler);
  auto cipher_bytes = ar::as_bytes<ar::DMRSA::block_enc_type>(cipher);
  EXPECT_EQ(cipher_bytes.size(),
            std::ceil(static_cast<float>(message_bytes.size()) / ar::size_of<ar::DMRSA::block_type>()) *
            ar::size_of<ar::DMRSA::block_enc_type>());

  auto decipher = rsa.decrypts(cipher_bytes);
  ASSERT_TRUE(decipher.has_value());
  // auto& decipher_bytes = decipher.value();
  auto decipher_bytes = ar::as_bytes<ar::DMRSA::block_type>(decipher.value(), filler);

  // fmt::println("Original  : {}", message_bytes);
  // fmt::println("Deciphered: {}", decipher_bytes);

  check_span_eq<u8, u8>(message_bytes, decipher_bytes);
}
