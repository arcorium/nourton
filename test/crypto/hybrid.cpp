#include <gtest/gtest.h>

#include "crypto/hybrid.h"
#include "util/algorithm.h"
#include "util/convert.h"

#include "util.h"

TEST(hybrid, encrypt)
{
  ar::DMRSA rsa{};
  auto kb = ar::random_bytes<1024>();
  auto enc_res = ar::encrypt(rsa.public_key(), kb);

  auto dec_res = ar::decrypt(rsa, enc_res.key_padding, enc_res.cipher_key, enc_res.data_padding,
                             enc_res.cipher_data);
  ASSERT_TRUE(dec_res.has_value());
  auto decipher_data = dec_res.value();

  check_span_eq<u8, u8>(kb, decipher_data);
}