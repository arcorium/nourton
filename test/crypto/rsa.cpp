#include <gtest/gtest.h>

#include "util.h"

#include "crypto/dm_rsa.h"

#include "util/convert.h"

// #inlcude "util.h"

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
  std::string_view message{"Lorem Ipsum"};
  auto message_bytes = ar::as_span(message);

  ar::RSA rsa{};
  auto [filler, cipher] = rsa.encrypts(message_bytes);
  EXPECT_EQ(cipher.size(), message_bytes.size() / ar::size_of<ar::RSA::block_type>() + filler);
  EXPECT_EQ(filler, 1);
  auto cipher_bytes = ar::as_bytes<ar::RSA::block_enc_type>(cipher);
  EXPECT_EQ(cipher_bytes.size(),
            (message_bytes.size() / ar::size_of<ar::RSA::block_type>() + filler) * ar::size_of<ar::RSA::block_enc_type>(
            ));

  auto decipher = rsa.decrypts(cipher_bytes);
  ASSERT_TRUE(decipher.has_value());
  // auto& decipher_bytes = decipher.value();
  auto decipher_bytes = ar::as_bytes<ar::RSA::block_type>(decipher.value(), filler);

  check_span_eq<u8, u8>(message_bytes, decipher_bytes);
}
