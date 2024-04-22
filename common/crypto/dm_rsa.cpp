//
// Created by mizzh on 4/21/2024.
//

#include "dm_rsa.h"

#include "logger.h"

#include "util/algorithm.h"

namespace ar
{
  RSA::RSA(prime_type p, prime_type q)
    : p_{p}, q_{q}, n_{p * q}
  {
    prime_type phi = (p_ - 1) * (q_ - 1);

    // e should be 1 < e < phi and gcd(phi, e) == 1, e = prime number | e and phi is coprime using gcd
    // NOTE: coprime is when two number only have '1' as their only common factor between them
    // public key is {e, n}
    // TODO: make e more bigger
    e_ = 2;
    while (e_ < phi)
    {
      if (gcd(e_, phi) == 1)
        break;
      ++e_;
    }

    // d * e mod phi = 1
    // NOTE: some web says d * e = 1 mod phi (THE SAME)
    // calculate using mod inverse to get the d
    // naive approach: check from 1 until N and check if the mod result is 1 (SO SLOW) O(n)
    // extended euclidean algorithm can be used because e and phi is co-prime
    // private key {d, n}
    auto d_result = mod_inverse<prime_type>(e_, phi);
    if (!d_result.has_value())
    {
      Logger::critical("failed to create d");
      std::abort();
    }
    d_ = d_result.value();
  }

  RSA::RSA()
    : RSA{
      ar::generate_prime_nth<prime_type>(ar::random<u64>(1000, 10000)),
      ar::generate_prime_nth<prime_type>(ar::random<u64>(1000, 10000))
    }
  {
  }

  u64 RSA::encrypt(block_type block) noexcept
  {
    // m^e mod n

    auto cipher = mod_exponential<prime_type>((block), e_, n_);
    return cipher;
  }

  u64 RSA::decrypt(block_enc_type block) noexcept
  {
    // c^d mod n
    auto message = mod_exponential<prime_type>(block, d_, n_);
    return message;
  }

  struct RSA::_public_key RSA::public_key() const noexcept
  {
    return _public_key{.e = e_, .n = n_};
  }

  struct RSA::_private_key RSA::private_key() const noexcept
  {
    return _private_key{.d = d_, .n = n_};
  }
}
