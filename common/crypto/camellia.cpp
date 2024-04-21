//
// Created by mizzh on 4/20/2024.
//

#include "camellia.h"

#include <array>
#include <ranges>

#include <fmt/std.h>
#include <fmt/ranges.h>

#include <util/convert.h>
#include <util/literal.h>

#include "util/make.h"

namespace ar
{
  Camellia::Camellia(key_type key) noexcept
  {
    u128 kl = rawToBoost_uint128(key.data());
    u128 kr = 0; // 0, becuase the key only 128 bit

    // std::cout << "KL: " << std::hex << kl << std::endl;
    // std::cout << "aa: " << std::dec << aa << std::endl;
    // fmt::println("Key: {}", key);

    auto XORED = kl ^ kr;
    // WARN: Need to check the convert result
    auto d1 = (XORED >> 64).convert_to<u64>();
    auto d2 = (XORED & MASK_64BIT).convert_to<u64>();

    d2 = d2 ^ F(d1, SIGMA[0]);
    d1 = d1 ^ F(d2, SIGMA[1]);
    d1 = d1 ^ (kl >> 64).convert_to<u64>();
    d2 = d2 ^ (kl & MASK_64BIT).convert_to<u64>();
    d2 = d2 ^ F(d1, SIGMA[2]);
    d1 = d1 ^ F(d2, SIGMA[3]);
    ka_ = d1;
    ka_ = (ka_ << 64) | d2;

    kw_[0] = (kl >> 64).convert_to<u64>();
    kw_[1] = (kl & MASK_64BIT).convert_to<u64>();
    k_[0] = (ka_ >> 64).convert_to<u64>();
    k_[1] = (ka_ & MASK_64BIT).convert_to<u64>();
    k_[2] = (ar::rotl(kl, 15) >> 64).convert_to<u64>();
    k_[3] = (ar::rotl(kl, 15) & MASK_64BIT).convert_to<u64>();
    k_[4] = (ar::rotl(ka_, 15) >> 64).convert_to<u64>();
    k_[5] = (ar::rotl(ka_, 15) & MASK_64BIT).convert_to<u64>();
    ke_[0] = (ar::rotl(ka_, 30) >> 64).convert_to<u64>();
    ke_[1] = (ar::rotl(ka_, 30) & MASK_64BIT).convert_to<u64>();
    k_[6] = (ar::rotl(kl, 45) >> 64).convert_to<u64>();
    k_[7] = (ar::rotl(kl, 45) & MASK_64BIT).convert_to<u64>();
    k_[8] = (ar::rotl(ka_, 45) >> 64).convert_to<u64>();
    k_[9] = (ar::rotl(kl, 60) & MASK_64BIT).convert_to<u64>();
    k_[10] = (ar::rotl(ka_, 60) >> 64).convert_to<u64>();
    k_[11] = (ar::rotl(ka_, 60) & MASK_64BIT).convert_to<u64>();
    ke_[2] = (ar::rotl(kl, 77) >> 64).convert_to<u64>();
    ke_[3] = (ar::rotl(kl, 77) & MASK_64BIT).convert_to<u64>();
    k_[12] = (ar::rotl(kl, 94) >> 64).convert_to<u64>();
    k_[13] = (ar::rotl(kl, 94) & MASK_64BIT).convert_to<u64>();
    k_[14] = (ar::rotl(ka_, 94) >> 64).convert_to<u64>();
    k_[15] = (ar::rotl(ka_, 94) & MASK_64BIT).convert_to<u64>();
    k_[16] = (ar::rotl(kl, 111) >> 64).convert_to<u64>();
    k_[17] = (ar::rotl(kl, 111) & MASK_64BIT).convert_to<u64>();
    kw_[2] = (ar::rotl(ka_, 111) >> 64).convert_to<u64>();
    kw_[3] = (ar::rotl(ka_, 111) & MASK_64BIT).convert_to<u64>();
  }

  std::expected<Camellia, std::string_view> Camellia::create(std::string_view key) noexcept
  {
    if (key.size() != KEY_BYTE)
      return std::unexpected<std::string_view>("key should be 16 bytes length");

    std::array<u8, KEY_BYTE> key_bytes{};
    for (auto&& [i, n] : key_bytes | std::ranges::views::enumerate)
      n = key[i];

    return Camellia{key_type{key_bytes}};
  }

  std::expected<std::array<unsigned char, KEY_BYTE>, std::string_view> Camellia::encrypt(
    block_type block) const noexcept
  {
    if (block.size() != KEY_BYTE)
      return std::unexpected("block should be 16 bytes long!");
    // auto text = to_128(block);
    auto text = rawToBoost_uint128(block.data());

    auto d2 = (text >> 64).convert_to<u64>();
    auto d1 = (text & MASK_64BIT).convert_to<u64>();

    auto round = [this](u64& d1, u64& d2, usize i) {
      for (std::size_t j = 0; j < 3; ++j)
      {
        auto x = i + (j * 2);
        d2 = d2 ^ F(d1, k_[x + 0]);
        d1 = d1 ^ F(d2, k_[x + 1]);
      }
    };

    // pre-whitening
    d1 = d1 ^ kw_[0];
    d2 = d2 ^ kw_[1];

    for (usize i = 0; i < 3; ++i)
    {
      round(d1, d2, i * 6);
      if (i < 2)
      {
        d1 = FL(d1, ke_[i * 2 + 0]);
        d2 = FLINV(d2, ke_[i * 2 + 1]);
      }
    }

    // post-whitening
    d2 = d2 ^ kw_[2];
    d1 = d1 ^ kw_[3];

    // return combine(d1, d2);
    return combine_to_bytes(d1, d2);
  }

  std::tuple<usize, std::vector<u8>> Camellia::encrypts(std::span<u8> bytes) const noexcept
  {
    // Check the modulo
    auto remaining = bytes.size() % KEY_BYTE;
    auto total_block = bytes.size() / KEY_BYTE;
    auto fill = KEY_BYTE - remaining;
    std::vector<u8> result{};
    result.reserve(bytes.size() + fill);

    // TODO: Fill with 0x00
    for (usize i = 0; i < total_block; ++i)
    {
      auto offset = i * KEY_BYTE;
      auto sub = bytes.subspan(offset, KEY_BYTE);
      // PERF: insert the result directly on vector instead of returning array
      auto cipher = encrypt(sub).value();
      result.insert_range(result.end(), cipher);
    }

    // Last block
    std::array<u8, 16> last_block{};
    std::memcpy(last_block.data(), bytes.data() + total_block * KEY_BYTE, remaining);
    auto cipher = encrypt(last_block).value();
    result.insert_range(result.end(), cipher);

    return std::make_tuple(fill, result);
  }

  std::expected<std::array<u8, KEY_BYTE>, std::string_view> Camellia::decrypt(block_type cipher_block) const noexcept
  {
    if (cipher_block.size() != KEY_BYTE)
      return std::unexpected("block should be 16 bytes long!");

    auto cipher = rawToBoost_uint128(cipher_block.data());

    auto d2 = (cipher >> 64).convert_to<u64>();
    auto d1 = (cipher & MASK_64BIT).convert_to<u64>();

    auto round = [this](u64& d1, u64& d2, usize i) {
      for (std::size_t j = 0; j < 3; ++j)
      {
        auto x = i - (j * 2);
        d2 = d2 ^ F(d1, k_[x]);
        d1 = d1 ^ F(d2, k_[x - 1]);
      }
    };

    // Pre whitening
    d1 = d1 ^ kw_[2];
    d2 = d2 ^ kw_[3];

    for (usize i = 0; i < 3; ++i)
    {
      round(d1, d2, (18 - i * 6) - 1);
      if (i < 2)
      {
        d1 = FL(d1, ke_[3 - (i * 2 + 0)]); // 3, 1
        d2 = FLINV(d2, ke_[3 - (i * 2 + 1)]); // 2, 0
      }
    }

    // Post whitening
    d2 = d2 ^ kw_[0];
    d1 = d1 ^ kw_[1];

    // return combine(d1, d2);
    return combine_to_bytes(d1, d2);
  }

  std::expected<std::vector<u8>, std::string_view> Camellia::decrypts(std::span<u8> bytes, usize garbage) const noexcept
  {
    if (bytes.size() % KEY_BYTE != 0)
      return std::unexpected("the ciphertext size is not multiple of 16"sv);

    std::vector<u8> result{};
    result.reserve(bytes.size());
    for (usize byte = 0; byte < bytes.size(); byte += 16)
    {
      auto a = bytes.subspan(byte, KEY_BYTE);
      auto decipher = decrypt(a).value();
      result.insert_range(result.end(), decipher);
    }

    if (garbage)
      result.resize(bytes.size() - garbage);
    return ar::make_expected<std::vector<u8>, std::string_view>(result);
  }

  u64 Camellia::F(u64 in, u64 ke) const noexcept
  {
    const u64 x = in ^ ke;
    u8 t1 = x >> 56;
    u8 t2 = (x >> 48) & MASK_8BIT;
    u8 t3 = (x >> 40) & MASK_8BIT;
    u8 t4 = (x >> 32) & MASK_8BIT;
    u8 t5 = (x >> 24) & MASK_8BIT;
    u8 t6 = (x >> 16) & MASK_8BIT;
    u8 t7 = (x >> 8) & MASK_8BIT;
    u8 t8 = x & MASK_8BIT;
    t1 = SBOX1[t1];
    t2 = SBOX2[t2];
    t3 = SBOX3[t3];
    t4 = SBOX4[t4];
    t5 = SBOX2[t5];
    t6 = SBOX3[t6];
    t7 = SBOX4[t7];
    t8 = SBOX1[t8];
    const u8 y1 = t1 ^ t3 ^ t4 ^ t6 ^ t7 ^ t8;
    const u8 y2 = t1 ^ t2 ^ t4 ^ t5 ^ t7 ^ t8;
    const u8 y3 = t1 ^ t2 ^ t3 ^ t5 ^ t6 ^ t8;
    const u8 y4 = t2 ^ t3 ^ t4 ^ t5 ^ t6 ^ t7;
    const u8 y5 = t1 ^ t2 ^ t6 ^ t7 ^ t8;
    const u8 y6 = t2 ^ t3 ^ t5 ^ t7 ^ t8;
    const u8 y7 = t3 ^ t4 ^ t5 ^ t6 ^ t8;
    const u8 y8 = t1 ^ t4 ^ t5 ^ t6 ^ t7;

    // from MSB -> LSB
    auto yy1 = static_cast<u64>(y1) << 56;
    auto yy2 = static_cast<u64>(y2) << 48;
    auto yy3 = static_cast<u64>(y3) << 40;
    auto yy4 = static_cast<u64>(y4) << 32;
    auto yy5 = static_cast<u64>(y5) << 24;
    auto yy6 = static_cast<u64>(y6) << 16;
    auto yy7 = static_cast<u64>(y7) << 8;
    auto yy8 = static_cast<u64>(y8);

    //uint64_t F_OUT = (y1 << 56) | (y2 << 48) | (y3 << 40) | (y4 << 32)
    //| (y5 << 24) | (y6 << 16) | (y7 << 8) | y8;
    uint64_t F_OUT2 = yy1 | yy2 | yy3 | yy4 | yy5 | yy6 | yy7 | yy8;

    return F_OUT2;
  }

  u64 Camellia::FL(u64 in, u64 subkey) const noexcept
  {
    u32 left = in >> 32;
    u32 right = in & MASK_32BIT;
    const u32 subkey_msb = subkey >> 32;
    const u32 subkey_lsb = subkey & MASK_32BIT;

    auto temp = right ^ std::rotl(left & subkey_msb, 1);
    right ^= std::rotl(left & subkey_msb, 1);
    left ^= (right | subkey_lsb);
    return combine<u64>(right, left);
  }

  u64 Camellia::FLINV(u64 in, u64 subkey) const noexcept
  {
    u32 left = in >> 32; // lsb
    u32 right = in & MASK_32BIT; //msb
    const u32 subkey_msb = subkey >> 32;
    const u32 subkey_lsb = subkey & MASK_32BIT;

    auto temp = right ^ std::rotl(left & subkey_msb, 1);
    left ^= (right | subkey_lsb);
    right ^= std::rotl(left & subkey_msb, 1);
    return combine<u64>(right, left);
  }
} // ar
