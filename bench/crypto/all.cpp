#include "util/convert.h"

#include <benchmark/benchmark.h>

#include <crypto/dm_rsa.h>
#include <crypto/rsa.h>
#include <crypto/camellia.h>
#include <crypto/aes.h>

#ifdef expect
  #undef expect
#endif

#define expect(val) \
  if (!(val))       \
    std::exit(-1);

static constexpr usize MB = 1024 * 1024;

static void dmrsa(benchmark::State& state)
{
  std::vector<u64> bytes{};
  auto val = state.range();
  bytes.resize(MB * val / sizeof(u64));
  std::ranges::fill(bytes, 0x20);

  for (auto _ : state)
  {
    // Key generation
    ar::DMRSA dmrsa{};
    auto bytes_bytes = ar::as_byte_span<u64>(bytes);
    // Encrypt
    auto [filler, cipher] = dmrsa.encrypts(bytes_bytes);
    // Decrypt
    auto cipher_bytes = ar::as_byte_span<ar::DMRSA::block_enc_type>(cipher);
    auto result = dmrsa.decrypts(cipher_bytes);
    expect(result.has_value());
  }
}

static void rsa(benchmark::State& state)
{
  std::vector<u64> bytes{};
  auto val = state.range();
  bytes.resize(MB * val / sizeof(u64));
  std::ranges::fill(bytes, 0x20);

  for (auto _ : state)
  {
    // Key generation
    ar::RSA rsa{};
    auto bytes_bytes = ar::as_byte_span<u64>(bytes);
    // Encrypt
    auto [filler, cipher] = rsa.encrypts(bytes_bytes);
    // Decrypt
    auto cipher_bytes = ar::as_byte_span<ar::DMRSA::block_enc_type>(cipher);
    auto result = rsa.decrypts(cipher_bytes);
    expect(result.has_value());
  }
}

static void camellia(benchmark::State& state)
{
  std::vector<u64> bytes{};
  auto val = state.range();
  bytes.resize(MB * val / sizeof(u64));
  std::ranges::fill(bytes, 0x20);
  auto bytes_bytes = ar::as_byte_span<u64>(bytes);

  for (auto _ : state)
  {
    // Key generation
    ar::Camellia camellia{};
    // Encrypt
    auto [filler, cipher] = camellia.encrypts(bytes_bytes);
    // Decrypt
    auto result = camellia.decrypts(cipher);
    expect(result.has_value());
  }
}

static void aes(benchmark::State& state)
{
  std::vector<u64> bytes{};
  auto val = state.range();
  bytes.resize(MB * val / sizeof(u64));
  std::ranges::fill(bytes, 0x20);
  auto bytes_bytes = ar::as_byte_span<u64>(bytes);

  for (auto _ : state)
  {
    // Key generation
    ar::AES aes{};
    // Encrypt
    auto [filler, cipher] = aes.encrypts(bytes_bytes);
    // Decrypt
    auto result = aes.decrypts(cipher);
    expect(result.has_value());
  }
}

static void hybrid(benchmark::State& state)
{
  std::vector<u64> bytes{};
  auto val = state.range();
  bytes.resize(MB * val / sizeof(u64));
  std::ranges::fill(bytes, 0x20);
  auto bytes_bytes = ar::as_byte_span<u64>(bytes);

  for (auto _ : state)
  {
    // DMRSA - key generation
    ar::DMRSA dmrsa{};

    // Camellia - key generation
    ar::Camellia camellia{};

    // DMRSA - encrypt symmetric key
    auto sym_key = camellia.key();
    std::vector<u8> sym_key_copy{sym_key.begin(), sym_key.end()};
    auto [key_filler, cipher_sym_key] = dmrsa.encrypts(sym_key_copy);

    // Camellia - encrypt
    auto [data_filler, cipher_data] = camellia.encrypts(bytes_bytes);

    // DMRSA - decrypt symmetric key
    auto cipher_sym_key_bytes = ar::as_byte_span<ar::DMRSA::block_enc_type>(cipher_sym_key);
    auto decipher_key = dmrsa.decrypts(cipher_sym_key_bytes);
    expect(decipher_key.has_value());

    auto decipher_key_bytes
        = ar::as_byte_span<ar::DMRSA::block_type>(decipher_key.value(), key_filler);

    expect(decipher_key_bytes.size() == ar::KEY_BYTE);

    // Camellia - decrypt data
    ar::Camellia::key_type keys{decipher_key_bytes};
    ar::Camellia data_decryptor{keys};
    auto decipher_data = data_decryptor.decrypts(cipher_data);
    expect(decipher_data.has_value());
  }
}

static constexpr i64 ARG_1 = 20;
static constexpr i64 ARG_2 = 400;
static constexpr i64 ARG_3 = 800;
static constexpr i64 ARG_4 = 1600;

BENCHMARK(dmrsa)->Unit(benchmark::kMillisecond)->Arg(ARG_1)->Arg(ARG_2)->Arg(ARG_3)->Arg(ARG_4);
BENCHMARK(rsa)->Unit(benchmark::kMillisecond)->Arg(ARG_1)->Arg(ARG_2)->Arg(ARG_3)->Arg(ARG_4);
BENCHMARK(camellia)->Unit(benchmark::kMillisecond)->Arg(ARG_1)->Arg(ARG_2)->Arg(ARG_3)->Arg(ARG_4);
BENCHMARK(aes)->Unit(benchmark::kMillisecond)->Arg(ARG_1)->Arg(ARG_2)->Arg(ARG_3)->Arg(ARG_4);
BENCHMARK(hybrid)->Unit(benchmark::kMillisecond)->Arg(ARG_1)->Arg(ARG_2)->Arg(ARG_3)->Arg(ARG_4);
