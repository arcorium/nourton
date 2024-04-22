#include <gtest/gtest.h>

#include "crypto/dm_rsa.h"

TEST(rsa, prime_key_generated)
{
  ar::RSA rsa{};
  // u16 message = 0xDEED;
  u64 message = 0xDEEDBEEF;
  auto cipher = rsa.encrypt(message);
  auto decipher = rsa.decrypt(cipher);

  EXPECT_EQ(message, decipher);
}

TEST(rsa, prime_key_inputted)
{
}
