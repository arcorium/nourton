#pragma once
#include <expected>
#include <span>

#include "util/types.h"

namespace ar
{
  class RSA
  {
  public:
    using prime_type = u64;
    using key_type = ar::uint<ar::size_of<prime_type>() * 2>::type;
    using block_type = u32;
    using block_enc_type = u64;

  public:
    struct _public_key
    {
      prime_type e;
      key_type n;
    };

    struct _private_key
    {
      prime_type d;
      key_type n;
    };


    RSA(prime_type p, prime_type q, prime_type e = 0);

    /**
     * Create RSA object instance with randomly generated for p and q
     */
    RSA();

    std::expected<block_enc_type, std::string_view> encrypt(block_type block) noexcept;
    std::tuple<usize, std::vector<RSA::block_enc_type>> encrypts(std::span<u8> bytes) noexcept;
    block_type decrypt(block_enc_type block) noexcept;
    std::expected<std::vector<RSA::block_type>, std::string_view> decrypts(std::span<u8> bytes) noexcept;

    [[nodiscard]] _public_key public_key() const noexcept;
    [[nodiscard]] _private_key private_key() const noexcept;

  private:
    prime_type p_;
    prime_type q_;
    key_type n_;
    prime_type e_;
    prime_type d_;
  };
}
