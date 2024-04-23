//
// Created by mizzh on 4/21/2024.
//

#include "dm_rsa.h"

#include <fmt/format.h>

#include "logger.h"

#include "util/algorithm.h"

namespace ar
{
  RSA::RSA(const prime_type p, const prime_type q, const prime_type e)
    : p_{p}, q_{q}, n_{p * q}, e_{e}
  {
    prime_type phi = (p_ - 1) * (q_ - 1);

    // e should be 1 < e < phi and gcd(phi, e) == 1, e = prime number | e and phi is coprime using gcd
    // NOTE: coprime is when two number only have '1' as their only common factor between them
    // public key is {e, n}
    // TODO: use ar::generate_primt_nth instead
    if (e == 0)
    {
      e_ = phi / 5;
      // e_ = 2;
      while (e_ < phi)
      {
        if (gcd(e_, phi) == 1)
          break;
        ++e_;
      }
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
      ar::nth_prime<prime_type>(ar::random<u64>(8000, 10000)),
      ar::nth_prime<prime_type>(ar::random<u64>(8000, 10000))
    }
  {
  }

  std::expected<RSA::block_enc_type, std::string_view> RSA::encrypt(block_type block) noexcept
  {
    // m^e mod n
    if (block >= n_)
      return std::unexpected("block should be less than n, choose bigger prime numbers!"sv);

    auto cipher = mod_exponential<key_type>((block), e_, n_);
    return cipher.convert_to<block_enc_type>();
  }

  std::tuple<usize, std::vector<RSA::block_enc_type>> RSA::encrypts(std::span<u8> bytes) noexcept
  {
    usize remaining_bytes = bytes.size() % ar::size_of<block_type>();
    usize block_count = bytes.size() / ar::size_of<block_type>();

    std::span<block_type> temp{reinterpret_cast<block_type*>(bytes.data()), block_count};
    std::vector<RSA::block_enc_type> result{};
    auto capacity = block_count + ((remaining_bytes) ? 1 : 0);
    result.reserve(capacity);

    for (const auto& val : temp)
    {
      auto cipher = encrypt(val);
      if (!cipher.has_value())
        Logger::critical(fmt::format("failed to encrypt using RSA: {}", cipher.error()));

      result.push_back(cipher.value());
    }

    // no remaining bytes needed to handle
    if (!remaining_bytes)
      return std::make_tuple(0, result);

    std::array<u8, ar::size_of<block_type>()> last_bytes{};
    last_bytes.fill(0x00);
    for (usize i = 0; i < remaining_bytes; ++i)
      last_bytes[i] = bytes[block_count * ar::size_of<block_type>() + i];

    auto cipher = encrypt(*reinterpret_cast<block_type*>(last_bytes.data()));

    if (!cipher.has_value())
      Logger::critical(fmt::format("failed to encrypt using RSA: {}", cipher.error()));

    result.push_back(cipher.value());

    return std::make_tuple(ar::size_of<block_type>() - remaining_bytes, result);
  }

  RSA::block_type RSA::decrypt(block_enc_type block) noexcept
  {
    // c^d mod n
    // PERF: maybe its not needed to use key_type and use prime_type instead?
    auto message = mod_exponential<key_type>(block, d_, n_);
    return message.convert_to<block_type>();
  }

  std::expected<std::vector<RSA::block_type>, std::string_view> RSA::decrypts(std::span<u8> bytes) noexcept
  {
    constexpr auto byte_size = ar::size_of<block_enc_type>();
    if (bytes.size() % byte_size != 0)
      return std::unexpected(fmt::format("cipher text should be multiple of {}", byte_size));

    usize block_count = bytes.size() / byte_size;

    std::span<block_enc_type> temp{(block_enc_type*)bytes.data(), block_count};
    std::vector<RSA::block_type> result{};
    result.reserve(block_count);

    for (const auto& val : temp)
    {
      auto decipher = decrypt(val);
      result.push_back(decipher);
    }
    return result;
  }

  RSA::_public_key RSA::public_key() const noexcept
  {
    return _public_key{.e = e_, .n = n_};
  }

  RSA::_private_key RSA::private_key() const noexcept
  {
    return _private_key{.d = d_, .n = n_};
  }
}
