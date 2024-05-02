//
// Created by mizzh on 4/21/2024.
//

#define AR_USE_BOOST_POWM
#include "dm_rsa.h"

#include <asio/io_service.hpp>

#include <fmt/format.h>

#include "logger.h"

#include "util/algorithm.h"
#include "util/convert.h"

namespace ar
{
  bool DMRSA::_public_key::is_valid() const noexcept
  {
    return !(e1_ == 0 || e2_ == 0 || n1_ == 0 || n2_ == 0);
  }

  bool DMRSA::_private_key::is_valid() const noexcept
  {
    return !(d1_ == 0 || d2_ == 0 || n1_ == 0 || n2_ == 0);
  }

  DMRSA::DMRSA(prime_type p1, prime_type p2, prime_type q1, prime_type q2, prime_type e1, prime_type e2) noexcept
    : p1_{p1}, p2_{p2}, q1_{q1}, q2_{q2}, n1_{p1 * p2}, n2_{q1 * q2}, e1_{e1}, e2_{e2}
  {
    // std::cout << "----------------------------------------" << std::endl;
    // std::cout << "p1: " << std::hex << p1_ << std::dec << " | " << p1_ << std::endl;
    // std::cout << "p2: " << std::hex << p2_ << std::dec << " | " << p2_ << std::endl;
    // std::cout << "n1: " << std::hex << n1_ << std::dec << " | " << n1_ << std::endl;
    // std::cout << "q1: " << std::hex << q1_ << std::dec << " | " << q1_ << std::endl;
    // std::cout << "q2: " << std::hex << q2_ << std::dec << " | " << q2_ << std::endl;
    // std::cout << "n2: " << std::hex << n2_ << std::dec << " | " << n2_ << std::endl;

    prime_type phi1 = (p1_ - 1) * (p2_ - 1);
    prime_type phi2 = (q1_ - 1) * (q2_ - 1);

    if (e1 == 0)
    {
      e1_ = phi1 / 5;
      while (e1_ < phi1)
      {
        if (gcd(e1_, phi1) == 1)
          break;
        ++e1_;
      }
    }

    if (e2 == 0)
    {
      e2_ = phi2 / 5;
      while (e2_ < phi2)
      {
        if (gcd(e2_, phi2) == 1)
          // if (gcd(e2_, phi2) == 2)
          break;
        ++e2_;
      }
    }

    auto d1_result = mod_inverse<prime_type>(e1_, phi1);
    if (!d1_result.has_value())
      Logger::critical("failed to create d1");

    // auto _phi2 = phi(phi2);
    auto d2_result = mod_inverse<prime_type>(e2_, phi2);
    if (!d2_result.has_value())
      Logger::critical("failed to create d2");

    d1_ = d1_result.value();
    d2_ = d2_result.value();

    // std::cout << "e1: " << std::hex << e1_ << std::dec << " | " << e1_ << std::endl;
    // std::cout << "e2: " << std::hex << e2_ << std::dec << " | " << e2_ << std::endl;
    // std::cout << "d1: " << std::hex << d1_ << std::dec << " | " << d1_ << std::endl;
    // std::cout << "d2: " << std::hex << d2_ << std::dec << " | " << d2_ << std::endl;
    // std::cout << "----------------------------------------" << std::endl;
  }

  DMRSA::DMRSA(const _public_key& public_key) noexcept
    : n1_{public_key.n1_}, n2_{public_key.n2_}, e1_{public_key.e1_}, e2_{public_key.e2_} {}

  DMRSA::DMRSA() noexcept
    : DMRSA{
      ar::nth_prime<prime_type>(ar::random<u64>(8000, 8000)),
      ar::nth_prime<prime_type>(ar::random<u64>(8001, 9000)),
      ar::nth_prime<prime_type>(ar::random<u64>(9001, 10000)),
      ar::nth_prime<prime_type>(ar::random<u64>(10001, 11000))
    } {}

  DMRSA::DMRSA(low_prime_t) noexcept
    : DMRSA{
      ar::nth_prime<prime_type>(ar::random<u64>(50, 100)),
      ar::nth_prime<prime_type>(ar::random<u64>(101, 200)),
      ar::nth_prime<prime_type>(ar::random<u64>(201, 250)),
      ar::nth_prime<prime_type>(ar::random<u64>(251, 300))
    } {}

  DMRSA::DMRSA(mid_prime_t) noexcept
    : DMRSA{
      ar::nth_prime<prime_type>(ar::random<u64>(1001, 1500)),
      ar::nth_prime<prime_type>(ar::random<u64>(1501, 2000)),
      ar::nth_prime<prime_type>(ar::random<u64>(2000, 2500)),
      ar::nth_prime<prime_type>(ar::random<u64>(2501, 3000))
    } {}

  std::expected<DMRSA::block_enc_type, std::string_view> DMRSA::encrypt(block_type block) noexcept
  {
    // (m^e1 mod n1)^e2 mod n2
    // (((m mod n1) (e1 mod n1)) mod n2) (e2 mod n2)
    if (e1_ == 0 || e2_ == 0)
      return std::unexpected("failed to encrypt due to the e1 and e2 value is 0");
    if (block >= n1_)
      return std::unexpected("block should be less than n1, choose bigger prime numbers!"sv);

    // std::cout << "block: " << std::hex << (int)block << " | " << std::dec << (int)block << std::endl;

    auto first = ar::mod_exponential<key_type>(block, e1_, n1_);
    // std::cout << "first: " << std::hex << first << " | " << std::dec << first << std::endl;

    if (first >= n2_)
      return std::unexpected("first result should be less than n2, choose bigger prime numbers!"sv);

    auto second = ar::mod_exponential<key_type>(first, e2_, n2_);
    // std::cout << "second: " << std::hex << second << " | " << std::dec << second << std::endl;

    auto result = second.convert_to<block_enc_type>();
    if (second != result)
      return std::unexpected("result has different value with second");
    return result;
  }

  std::tuple<usize, std::vector<DMRSA::block_enc_type>> DMRSA::encrypts(std::span<u8> bytes) noexcept
  {
    // NOTE: Implementation is the same with original RSA
    usize remaining_bytes = bytes.size() % ar::size_of<block_type>();
    usize block_count = bytes.size() / ar::size_of<block_type>();

    std::span<block_type> temp{reinterpret_cast<block_type*>(bytes.data()), block_count};
    std::vector<block_enc_type> result{};
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
    for (usize j = 0; j < remaining_bytes; ++j)
      last_bytes[j] = bytes[block_count * ar::size_of<block_type>() + j];

    auto cipher = encrypt(*reinterpret_cast<block_type*>(last_bytes.data()));

    if (!cipher.has_value())
      Logger::critical(fmt::format("failed to encrypt using RSA: {}", cipher.error()));

    result.push_back(cipher.value());

    return std::make_tuple(ar::size_of<block_type>() - remaining_bytes, result);
  }

  DMRSA::block_type DMRSA::decrypt(block_enc_type block) noexcept
  {
    // (c^d2 mod n1)^d1 mod n2
    // (((c mod n2) (e1 mod n2)) mod n1) (d1 mod n1)
    // std::cout << "cipher: " << std::hex << block << " | " << std::dec << block << std::endl;

    auto first = ar::mod_exponential<key_type>(block, d2_, n2_);
    // std::cout << "first: " << std::hex << first << " | " << std::dec << first << std::endl;

    auto second = ar::mod_exponential<key_type>(first, d1_, n1_);
    // std::cout << "second: " << std::hex << second << " | " << std::dec << second << std::endl;

    return second.convert_to<block_type>();
  }

  std::expected<std::vector<DMRSA::block_type>, std::string_view> DMRSA::decrypts(std::span<const u8> bytes) noexcept
  {
    constexpr auto byte_size = ar::size_of<block_enc_type>();
    if (bytes.size() % byte_size != 0)
      return std::unexpected(fmt::format("cipher text should be multiple of {}", byte_size));

    usize block_count = bytes.size() / byte_size;

    std::span<const block_enc_type> temp{reinterpret_cast<const block_enc_type*>(bytes.data()), block_count};
    std::vector<block_type> result{};
    result.reserve(block_count);

    for (const auto& val : temp)
    {
      auto decipher = decrypt(val);
      result.push_back(decipher);
    }
    return result;
  }

  DMRSA::_public_key DMRSA::public_key() const noexcept
  {
    return _public_key{
      .e1_ = e1_,
      .e2_ = e2_,
      .n1_ = n1_,
      .n2_ = n2_
    };
  }

  DMRSA::_private_key DMRSA::private_key() const noexcept
  {
    return _private_key{
      .d1_ = d1_,
      .d2_ = d2_,
      .n1_ = n1_,
      .n2_ = n2_
    };
  }

  std::vector<u8> serialize(const DMRSA::_public_key& key) noexcept
  {
    auto n1_bytes = as_bytes(key.n1_);
    auto n2_bytes = as_bytes(key.n2_);
    auto e1_bytes = as_span<u8, DMRSA::prime_type>(key.e1_);
    auto e2_bytes = as_span<u8, DMRSA::prime_type>(key.e2_);

    std::vector<u8> result{};
    result.append_range(e1_bytes);
    result.append_range(e2_bytes);
    result.append_range(n1_bytes);
    result.append_range(n2_bytes);
    return result;
  }

  std::expected<DMRSA::_public_key, std::string_view> deserialize(std::span<const u8> bytes) noexcept
  {
    constexpr static usize size = ar::size_of<DMRSA::key_type>() * 2 + ar::size_of<DMRSA::prime_type>() * 2;
    constexpr static usize prime_size = ar::size_of<DMRSA::prime_type>();
    constexpr static usize key_size = ar::size_of<DMRSA::key_type>();

    if (bytes.size() != size)
      return std::unexpected("provided bytes has different size from expected"sv);

    return DMRSA::_public_key{
      .e1_ = *((DMRSA::prime_type*)(bytes.data() + 0)),
      .e2_ = *((DMRSA::prime_type*)(bytes.data() + prime_size)),
      .n1_ = rawToBoost_uint128(bytes.data() + 2 * prime_size),
      .n2_ = rawToBoost_uint128(bytes.data() + 2 * prime_size + key_size),
    };
  }
}
