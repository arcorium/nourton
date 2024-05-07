#include "crypto/rsa.h"

#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <gtest/gtest.h>

#include "crypto/camellia.h"
#include "crypto/dm_rsa.h"
#include "util.h"
#include "util/algorithm.h"
#include "util/convert.h"
#include "util/file.h"

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
  std::string_view message{
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque placerat."};
  auto message_bytes = ar::as_span(message);
  usize expected_filler = 0;
  if (auto result = message_bytes.size() % ar::size_of<ar::RSA::block_type>())
    expected_filler = ar::size_of<ar::RSA::block_type>() - result;

  ar::RSA rsa{};
  auto [filler, cipher] = rsa.encrypts(message_bytes);
  EXPECT_EQ(cipher.size(),
            message_bytes.size() / ar::size_of<ar::RSA::block_type>() + (expected_filler ? 1 : 0));
  EXPECT_EQ(filler, expected_filler);
  auto cipher_bytes = ar::as_byte_span<ar::RSA::block_enc_type>(cipher);
  EXPECT_EQ(cipher_bytes.size(),
            std::ceil(static_cast<float>(message_bytes.size()) / ar::size_of<ar::RSA::block_type>())
                * ar::size_of<ar::RSA::block_enc_type>());

  auto decipher = rsa.decrypts(cipher_bytes);
  ASSERT_TRUE(decipher.has_value());
  // auto& decipher_bytes = decipher.value();
  auto decipher_bytes = ar::as_byte_span<ar::RSA::block_type>(decipher.value(), filler);

  check_span_eq<u8, u8>(message_bytes, decipher_bytes);
}

TEST(rsa, encrypt_file)
{
  auto plain_result = ar::read_file_as_bytes("../../resource/image/docs.png");
  ASSERT_TRUE(plain_result.has_value());

  for (usize i = 0; i < 50; ++i)
  {
    ar::RSA rsa{};
    auto [filler, cipher] = rsa.encrypts(plain_result.value());
    auto cipher_bytes = ar::as_byte_span<ar::RSA::block_enc_type>(cipher);
    auto decipher = rsa.decrypts(cipher_bytes);
    ASSERT_TRUE(decipher.has_value());
    // resize
    auto decipher_bytes = ar::as_byte_span<ar::RSA::block_type>(decipher.value(), filler);
    check_span_eq<u8, u8>(decipher_bytes, plain_result.value());
  }
}

TEST(rsa, serialize_public_key)
{
  for (usize i = 0; i < 20; ++i)
  {
    ar::RSA rsa{};
    auto public_key = rsa.public_key();
    auto serialized = ar::serialize(public_key);
    auto deserialized = ar::deserialize_rsa(serialized);
    ASSERT_TRUE(deserialized.has_value());
    ASSERT_EQ(deserialized->e, public_key.e);
    ASSERT_EQ(deserialized->n, public_key.n);
  }
}

TEST(dm_rsa, prime_key_inputted)
{
  ar::DMRSA rsa{13, 23, 11, 29};
  u8 message = 0xDE;
  auto cipher = rsa.encrypt(message);
  ASSERT_TRUE(cipher.has_value());

  auto decipher = rsa.decrypt(cipher.value());
  ASSERT_EQ(decipher, message);

  u16 bigger_message = 0xDEED;
  cipher = rsa.encrypt(bigger_message);
  ASSERT_FALSE(cipher.has_value());
}

TEST(dm_rsa, prime_key_generated)
{
  for (usize i = 0; i < 1000; ++i)
  {
    ar::DMRSA rsa{ar::DMRSA::low_prime};
    u8 message = 0xDE;
    auto cipher = rsa.encrypt(message);
    ASSERT_TRUE(cipher.has_value());

    auto decipher = rsa.decrypt(cipher.value());
    ASSERT_EQ(decipher, message);
  }

  for (usize i = 0; i < 100; ++i)
  {
    ar::DMRSA rsa{ar::DMRSA::mid_prime};
    u16 message = 0xDEED;
    auto cipher = rsa.encrypt(message);
    ASSERT_TRUE(cipher.has_value());

    auto decipher = rsa.decrypt(cipher.value());
    ASSERT_EQ(decipher, message);
  }

  for (usize i = 0; i < 50; ++i)
  {
    ar::DMRSA rsa{};
    // u32 message = 0xDEEDBEEF;
    auto message = std::numeric_limits<ar::DMRSA::block_type>::max();  // max value
    auto cipher = rsa.encrypt(message);
    ASSERT_TRUE(cipher.has_value());

    auto decipher = rsa.decrypt(cipher.value());
    ASSERT_EQ(decipher, message);
  }
}

TEST(dm_rsa, encrypt_bytes)
{
  std::string_view message{
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque placerat."};
  auto message_bytes = ar::as_span(message);
  usize expected_filler = 0;
  if (auto result = message.size() % ar::size_of<ar::DMRSA::block_type>())
    expected_filler = ar::size_of<ar::DMRSA::block_type>() - result;

  ar::DMRSA rsa{};
  auto [filler, cipher] = rsa.encrypts(message_bytes);
  EXPECT_EQ(cipher.size(), message_bytes.size() / ar::size_of<ar::DMRSA::block_type>()
                               + (expected_filler ? 1 : 0));
  EXPECT_EQ(filler, expected_filler);
  auto cipher_bytes = ar::as_byte_span<ar::DMRSA::block_enc_type>(cipher);
  EXPECT_EQ(cipher_bytes.size(), std::ceil(static_cast<float>(message_bytes.size())
                                           / ar::size_of<ar::DMRSA::block_type>())
                                     * ar::size_of<ar::DMRSA::block_enc_type>());

  auto decipher = rsa.decrypts(cipher_bytes);
  ASSERT_TRUE(decipher.has_value());
  auto decipher_bytes = ar::as_byte_span<ar::DMRSA::block_type>(decipher.value(), filler);

  check_span_eq<u8, u8>(message_bytes, decipher_bytes);
}

TEST(dm_rsa, encrypt_bytes_with_remainder)
{
  std::string_view message{
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque placerat.abc"};
  auto message_bytes = ar::as_span(message);
  usize expected_filler = 0;
  if (auto result = message.size() % ar::size_of<ar::DMRSA::block_type>())
    expected_filler = ar::size_of<ar::DMRSA::block_type>() - result;

  ar::DMRSA rsa{};
  auto [filler, cipher] = rsa.encrypts(message_bytes);
  EXPECT_EQ(cipher.size(), message_bytes.size() / ar::size_of<ar::DMRSA::block_type>()
                               + (expected_filler ? 1 : 0));
  EXPECT_EQ(filler, expected_filler);
  auto cipher_bytes = ar::as_byte_span<ar::DMRSA::block_enc_type>(cipher);
  EXPECT_EQ(cipher_bytes.size(), std::ceil(static_cast<float>(message_bytes.size())
                                           / ar::size_of<ar::DMRSA::block_type>())
                                     * ar::size_of<ar::DMRSA::block_enc_type>());

  auto decipher = rsa.decrypts(cipher_bytes);
  ASSERT_TRUE(decipher.has_value());
  auto decipher_bytes = ar::as_byte_span<ar::DMRSA::block_type>(decipher.value(), filler);

  check_span_eq<u8, u8>(message_bytes, decipher_bytes);
}

TEST(boost_multiprecision, number_to_bytes)
{
  u128 number{0x1122334455667788};
  u128 number_1{"0x11223344556677889900aabbccddeeff"};

  // std::cout << std::hex << "Number: " << number << std::endl;
  // std::cout << std::hex << "Number 1: " << number_1 << std::endl;

  auto bytes = ar::as_byte_span(number);
  auto bytes1 = ar::as_byte_span(number_1);

  // boost::multiprecision::export_bits(number_1, std::back_inserter(bytes), 8);

  auto number_2 = ar::rawToBoost_uint128(bytes.data());
  auto number_3 = ar::rawToBoost_uint128(bytes1.data());

  // std::cout << std::hex << "Number 2: " << number_2 << std::endl;

  ASSERT_EQ(number, number_2);
  ASSERT_EQ(number_3, number_1);
}

TEST(dm_rsa, serialization)
{
  ar::DMRSA rsa{};

  auto pk = rsa.public_key();
  auto pk_bytes = ar::serialize(pk);
  EXPECT_EQ(pk_bytes.size(),
            (2 * ar::size_of<ar::DMRSA::prime_type>() + 2 * ar::size_of<ar::DMRSA::key_type>()));

  auto decipher_pk = ar::deserialize(pk_bytes);
  ASSERT_TRUE(decipher_pk.has_value());
  pk_bytes.emplace_back(0);
  auto temp_pk = ar::deserialize(pk_bytes);
  EXPECT_FALSE(temp_pk.has_value());

  EXPECT_EQ(decipher_pk->e1, pk.e1);
  EXPECT_EQ(decipher_pk->e2, pk.e2);
  EXPECT_EQ(decipher_pk->n1, pk.n1);
  EXPECT_EQ(decipher_pk->n2, pk.n2);
}

TEST(dm_rsa, encrypt_camellia_key)
{
  for (usize i = 0; i < 20; ++i)
  {
    auto original_key = ar::random_bytes<16>();
    // auto original_key = std::span<u8, 16>(ar::as_span(key_str));
    ar::Camellia camellia{original_key};

    ar::DMRSA rsa{};

    auto [filler, cipher] = rsa.encrypts(original_key);
    auto cipher_bytes = ar::as_byte_span<u64>(cipher);
    auto decipher_key = rsa.decrypts(cipher_bytes);
    ASSERT_TRUE(decipher_key.has_value());
    auto decipher_key_bytes = ar::as_byte_span<ar::DMRSA::block_type>(decipher_key.value(), filler);

    // check_span_eq<u8, u8>(original_key, decipher_key.value());
    check_span_eq<u8, u8>(original_key, decipher_key_bytes);
  }
}

TEST(dm_rsa, serialize_public_key)
{
  for (usize i = 0; i < 20; ++i)
  {
    ar::DMRSA dm_rsa{};
    auto public_key = dm_rsa.public_key();
    auto serialized = ar::serialize(public_key);
    auto deserialized = ar::deserialize(serialized);
    ASSERT_TRUE(deserialized.has_value());
    ASSERT_EQ(deserialized->e1, public_key.e1);
    ASSERT_EQ(deserialized->e2, public_key.e2);
    ASSERT_EQ(deserialized->n1, public_key.n1);
    ASSERT_EQ(deserialized->n2, public_key.n2);
  }
}
