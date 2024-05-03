#pragma once
#include <expected>
#include <span>

#include "util/types.h"

namespace ar
{
  class DMRSA
  {
  public:
    using prime_type = u64;
    using key_type = ar::uint<ar::size_of<prime_type>() * 2>::type;
    using block_type = u32;
    using block_enc_type = u64;

    struct _public_key
    {
      prime_type e1;
      prime_type e2;
      key_type n1;
      key_type n2;

      bool is_valid() const noexcept;
    };

    struct _private_key
    {
      prime_type d1;
      prime_type d2;
      key_type n1;
      key_type n2;

      bool is_valid() const noexcept;
    };

    struct low_prime_t
    {
    };

    struct mid_prime_t
    {
    };

    inline static low_prime_t low_prime;
    inline static mid_prime_t mid_prime;

  public:
    DMRSA(prime_type p1, prime_type p2, prime_type q1, prime_type q2, prime_type e1 = 0,
          prime_type e2 = 0) noexcept;
    // This constructor should only use for encrypting
    explicit DMRSA(const _public_key& public_key) noexcept;
    DMRSA() noexcept;
    explicit DMRSA(low_prime_t) noexcept;  // debug purpose
    explicit DMRSA(mid_prime_t) noexcept;  // debug purpose

    std::expected<block_enc_type, std::string_view> encrypt(block_type block) noexcept;
    std::tuple<usize, std::vector<block_enc_type>> encrypts(std::span<u8> bytes) noexcept;
    block_type decrypt(block_enc_type block) noexcept;
    std::expected<std::vector<block_type>, std::string_view> decrypts(
        std::span<const u8> bytes) noexcept;

    [[nodiscard]] _public_key public_key() const noexcept;
    [[nodiscard]] _private_key private_key() const noexcept;

  private:
    prime_type p1_;
    prime_type p2_;
    prime_type q1_;
    prime_type q2_;
    key_type n1_;
    key_type n2_;
    prime_type e1_;
    prime_type e2_;
    prime_type d1_;
    prime_type d2_;
  };

  std::vector<u8> serialize(const DMRSA::_public_key& key) noexcept;
  std::expected<DMRSA::_public_key, std::string_view> deserialize(
      std::span<const u8> bytes) noexcept;
}  // namespace ar
