#pragma once
#include <span>

#include "util/types.h"

namespace ar
{
  class RSA
  {
    using prime_type = u64;
    using block_type = u32;
    using block_enc_type = u32;

  public:
    struct _public_key
    {
      prime_type e;
      prime_type n;
    };

    struct _private_key
    {
      prime_type d;
      prime_type n;
    };


    RSA(prime_type p, prime_type q);

    /**
     * Create RSA object instance with randomly generated for p and q
     */
    RSA();

    u64 encrypt(block_type block) noexcept;
    // std::vector<u8> encrypts(std::span<u8> bytes) noexcept;
    u64 decrypt(block_enc_type block) noexcept;
    // std::vector<u8> decrypts(std::span<u8> bytes) noexcept;

    [[nodiscard]] _public_key public_key() const noexcept;
    [[nodiscard]] _private_key private_key() const noexcept;

  private:
    prime_type p_;
    prime_type q_;
    prime_type n_;
    prime_type e_;
    prime_type d_;
  };
}
