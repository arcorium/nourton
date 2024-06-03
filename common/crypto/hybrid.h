#pragma once

#include "camellia.h"
#include "dm_rsa.h"
#include "util/algorithm.h"
#include "util/convert.h"
#include "util/file.h"


namespace ar
{
  using asymm_type = ar::DMRSA;
  using symm_type = ar::Camellia;

  struct EncryptHybridResult
  {
    u8 data_padding;
    u8 key_padding;
    std::vector<u8> cipher_data;
    // std::vector<asymm_type::block_enc_type> cipher_key;
    std::vector<u8> cipher_key;
  };

  [[nodiscard]] static EncryptHybridResult encrypt(const DMRSA::_public_key& public_key,
                                                   std::span<u8> data) noexcept
  {
    // Encrypt file
    auto symmetric_key = random_bytes<KEY_BYTE>();
    Camellia symmetric{symmetric_key};
    auto [data_padding, enc_data] = symmetric.encrypts(data);

    // Encrypt symmetric key
    DMRSA rsa{public_key};
    auto [key_padding, enc_key] = rsa.encrypts(symmetric_key);
    auto enc_key_bytes = ar::as_byte_span<asymm_type::block_enc_type>(enc_key);

    return EncryptHybridResult{
        .data_padding = static_cast<u8>(data_padding),
        .key_padding = static_cast<u8>(key_padding),
        .cipher_data = std::move(enc_data),
        .cipher_key = std::vector<u8>{enc_key_bytes.begin(), enc_key_bytes.end()}
    };
  }

  [[nodiscard]] static std::expected<std::vector<u8>, std::string_view> decrypt(
      asymm_type& asymm, u8 key_padding, std::span<u8> cipher_key, u8 data_padding,
      std::span<u8> cipher_data) noexcept
  {
    // Decrypt key
    auto decipher_key_result = asymm.decrypts(cipher_key);
    if (!decipher_key_result)
      return std::unexpected(decipher_key_result.error());

    auto decipher_key_bytes = as_byte_span<DMRSA::block_type>(
        decipher_key_result.value(), key_padding);

    if (decipher_key_bytes.size() != KEY_BYTE)
      return std::unexpected("decrypted key is malformed");

    // Decrypt files
    Camellia::key_type key{decipher_key_bytes};
    Camellia symmetric_encryptor{key};
    auto decipher_file_result = symmetric_encryptor.decrypts(cipher_data, data_padding);
    if (!decipher_file_result)
      return std::unexpected(decipher_file_result.error());

    return ar::make_expected<std::vector<u8>, std::string_view>(
        std::move(decipher_file_result.value()));
  }
}