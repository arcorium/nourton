#pragma once

#include <aes/AES.h>

#include <expected>
#include <span>

#include "util/types.h"

namespace ar
{
  class AES
  {
  public:
    constexpr static u8 KEY_BYTE = 16;
    using block_type = std::span<u8>;
    using enc_block_type = block_type;

    enum class OperationMode
    {
      ECB,
      CBC
    };

    // key is randomly generated
    AES() noexcept;

    static std::expected<AES, std::string_view> create(std::span<u8> key) noexcept;

    std::expected<std::vector<u8>, std::string_view> encrypt(block_type block) noexcept;
    std::tuple<usize, std::vector<u8>> encrypts(std::span<u8> bytes) noexcept;

    std::expected<std::vector<u8>, std::string_view> decrypt(enc_block_type cipher_block) noexcept;
    std::expected<std::vector<u8>, std::string_view> decrypts(std::span<u8> cipher_bytes,
                                                              usize filler = 0) noexcept;

    [[nodiscard]] std::span<const u8> key() const noexcept;
    std::span<u8> key() noexcept;

  private:
    explicit AES(std::span<u8> key) noexcept;

  private:
    constexpr static AESKeyLength key_length_ = AESKeyLength::AES_128;
    constexpr static OperationMode mode_ = OperationMode::ECB;

    std::vector<u8> key_;
    ::AES aes_;
  };
}  // namespace ar
