#include "message/payload.h"

#include <gtest/gtest.h>

#include "../crypto/util.h"

#include <vector>


TEST(payload, serialize)
{
  auto bytes = ar::random_bytes<25>();
  ar::Camellia camellia = ar::Camellia::create();
  // Encrypt
  auto keys = camellia.key();

  // Encrypt with asymm
  std::vector<u8> keys_vector{keys.begin(), keys.end()};
  // Encrypt with symm
  std::vector<u8> byte_vector{bytes.begin(), bytes.end()};
  ar::SendFilePayload2::Data symm{
      .file_size = bytes.size(),
      .filename = "hello.jpg",
      .files = std::move(byte_vector)
  };

  auto symm_serialized = symm.serialize();

  ar::SendFilePayload2 payload{
      .key_padding = static_cast<u8>(9),
      .data_padding = static_cast<u8>(9),
      .key = keys_vector, // copy
      .data = std::move(symm_serialized),
  };

  auto payload_serialized = payload.serialize();
  std::error_code ec;
  auto deserialized_payload = alpaca::deserialize<ar::SendFilePayload2>(payload_serialized, ec);
  ASSERT_FALSE(ec);

  ASSERT_EQ(deserialized_payload.data_padding, payload.data_padding);
  ASSERT_EQ(deserialized_payload.key_padding, payload.key_padding);
  check_span_eq<u8, u8>(deserialized_payload.key, payload.key);
  check_span_eq<u8, u8>(deserialized_payload.data, payload.data);

  auto symm_deserialized = alpaca::deserialize<ar::SendFilePayload2::Data>(
      deserialized_payload.data, ec);
  ASSERT_FALSE(ec);
  ASSERT_EQ(symm_deserialized.file_size, symm.file_size);
  ASSERT_EQ(symm_deserialized.filename, symm.filename);
  check_span_eq<u8, u8>(symm_deserialized.files, symm.files);
};


TEST(payload, hybrid)
{
  auto bytes = ar::random_bytes<25>();
  ar::Camellia camellia = ar::Camellia::create();
  ar::DMRSA rsa{};

  // Encrypt
  auto keys = camellia.key();

  // Encrypt with symm
  ar::SendFilePayload2::Data data{
      .file_size = bytes.size(),
      .filename = "hello.jpg",
      .files = std::vector<u8>{bytes.begin(), bytes.end()}
  };

  auto data_serialized = data.serialize();

  auto [symm_padding, symm_enc] = camellia.encrypts(data_serialized);
  auto [key_padding, key_cipher] = rsa.encrypts(keys);
  auto key_cipher_bytes = ar::as_byte_span<ar::DMRSA::block_enc_type>(key_cipher);

  ar::SendFilePayload2 payload{
      .key_padding = static_cast<u8>(symm_padding),
      .data_padding = static_cast<u8>(key_padding),
      .key = std::vector<u8>{key_cipher_bytes.begin(), key_cipher_bytes.end()},
      .data = std::vector<u8>{symm_enc.begin(), symm_enc.end()}
  };

  auto payload_serialized = payload.serialize();

  // Deserialize
  std::error_code ec;
  auto payload_deserialized = alpaca::deserialize<ar::SendFilePayload2>(payload_serialized, ec);
  ASSERT_FALSE(ec);

  check_span_eq<u8, u8>(payload_deserialized.key, payload.key);

  auto key_decipher_result = rsa.decrypts(payload_deserialized.key);
  ASSERT_TRUE(key_decipher_result);

  auto key_dechiper_bytes = ar::as_byte_span<ar::DMRSA::block_type>(key_decipher_result.value(),
    payload_deserialized.key_padding);

  ar::Camellia decryptor{ar::Camellia::key_type{key_dechiper_bytes}};

  // Symm
  auto data_decipher = decryptor.decrypts(payload_deserialized.data,
                                          payload_deserialized.key_padding);
  ASSERT_TRUE(data_decipher);

  auto symm_deserialized = alpaca::deserialize<ar::SendFilePayload2::Data>(
      data_decipher.value(), ec);
  ASSERT_FALSE(ec);

  ASSERT_EQ(symm_deserialized.file_size, data.file_size);
  ASSERT_EQ(symm_deserialized.filename, data.filename);
  check_span_eq<u8, u8>(symm_deserialized.files, data.files);
};

TEST(payload, hybrid_func)
{
  auto bytes = ar::random_bytes<25>();
  ar::DMRSA rsa{};

  // Encrypt with symm
  ar::SendFilePayload2::Data data{
      .file_size = bytes.size(),
      .filename = "hello.jpg",
      .files = std::vector<u8>{bytes.begin(), bytes.end()}
  };
  auto data_serialized = data.serialize();

  auto result = ar::encrypt(rsa.public_key(), data_serialized);

  ar::SendFilePayload2 payload{
      .key_padding = result.key_padding,
      .data_padding = result.data_padding,
      .key = result.cipher_key, // copy
      .data = result.cipher_data // copy
  };

  auto payload_serialized = payload.serialize();

  // decipher and deserialize
  auto payload_result = ar::parse_body<ar::SendFilePayload2>(payload_serialized);
  ASSERT_TRUE(payload_result);

  auto data_result = ar::decrypt(rsa, payload_result->key_padding, payload_result->key,
                                 payload_result->data_padding, payload_result->data);
  ASSERT_TRUE(data_result);

  auto data_payload = ar::parse_body<ar::SendFilePayload2::Data>(data_result.value());
  ASSERT_TRUE(data_payload);

  ASSERT_EQ(data_payload->file_size, data.file_size);
  ASSERT_EQ(data_payload->filename, data.filename);
  check_span_eq<u8, u8>(data_payload->files, bytes);
}