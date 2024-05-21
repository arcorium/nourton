#include "util/algorithm.h"
#include "util/convert.h"
#include "util/file.h"

#include <benchmark/benchmark.h>

#include <crypto/dm_rsa.h>
#include <crypto/rsa.h>
#include <crypto/camellia.h>
#include <crypto/aes.h>

#include "util.h"

#ifdef expect
  #undef expect
#endif

#define expect(val) \
  if (!(val))       \
    std::exit(-1);

static constexpr usize MB = 1024 * 1024;

static ar::DMRSA dmrsa{};

static void decrypt_dmrsa(benchmark::State& state)
{
  std::vector<u64> bytes{};
  auto val = state.range();
  bytes.resize(MB * val / sizeof(u64));
  std::ranges::fill(bytes, 0x20);
  auto bytes_byte = ar::as_byte_span<u64>(bytes);
  auto result = dmrsa.encrypts(bytes_byte);

  for (auto _ : state)
  {
    auto cipher_bytes = ar::as_byte_span<ar::DMRSA::block_enc_type>(std::get<1>(result));
    auto decipher = dmrsa.decrypts(cipher_bytes);
    expect(decipher.has_value());
    benchmark::DoNotOptimize(result);
  }
}

static ar::RSA rsa{};

static void decrypt_rsa(benchmark::State& state)
{
  std::vector<u64> bytes{};
  auto val = state.range();
  bytes.resize(MB * val / sizeof(u64));
  std::ranges::fill(bytes, 0x20);
  auto bytes_byte = ar::as_byte_span<u64>(bytes);
  auto result = rsa.encrypts(bytes_byte);

  for (auto _ : state)
  {
    auto cipher_bytes = ar::as_byte_span<ar::DMRSA::block_enc_type>(std::get<1>(result));
    auto decipher = rsa.decrypts(cipher_bytes);
    expect(decipher.has_value());
    benchmark::DoNotOptimize(result);
  }
}

static ar::Camellia camellia{};

static void decrypt_camellia(benchmark::State& state)
{
  std::vector<u64> bytes{};
  auto val = state.range();
  bytes.resize(MB * val / sizeof(u64));
  std::ranges::fill(bytes, 0x20);
  auto bytes_byte = ar::as_byte_span<u64>(bytes);
  auto result = camellia.encrypts(bytes_byte);

  for (auto _ : state)
  {
    auto decipher = camellia.decrypts(std::get<1>(result));
    expect(decipher.has_value());
    benchmark::DoNotOptimize(result);
  }
}

static ar::AES aes{};

static void decrypt_aes(benchmark::State& state)
{
  std::vector<u64> bytes{};
  auto val = state.range();
  bytes.resize(MB * val / sizeof(u64));
  std::ranges::fill(bytes, 0x20);
  auto bytes_byte = ar::as_byte_span<u64>(bytes);
  auto result = aes.encrypts(bytes_byte);

  for (auto _ : state)
  {
    auto decipher = aes.decrypts(std::get<1>(result));
    expect(decipher.has_value());
    benchmark::DoNotOptimize(result);
  }
}

static void decrypt_hybrid(benchmark::State& state)
{
  std::vector<u64> data_bytes{};
  auto val = state.range();
  data_bytes.resize(MB * val / sizeof(u64));
  std::ranges::fill(data_bytes, 0x20);

  auto key_bytes = camellia.key();
  std::vector<u8> key{key_bytes.begin(), key_bytes.end()};

  // Encrypt key
  auto result_key = dmrsa.encrypts(key);
  auto cipher_key_bytes = ar::as_byte_span<ar::DMRSA::block_enc_type>(std::get<1>(result_key));
  // Encrypt data
  auto bytes_byte = ar::as_byte_span<u64>(data_bytes);
  auto result = camellia.encrypts(bytes_byte);

  for (auto _ : state)
  {
    // decipher key
    auto decipher_key = dmrsa.decrypts(cipher_key_bytes);
    expect(decipher_key.has_value());

    auto decipher_key_bytes
        = ar::as_byte_span<ar::DMRSA::block_type>(decipher_key.value(), std::get<0>(result_key));

    expect(decipher_key_bytes.size() == ar::KEY_BYTE);

    // decipher data
    ar::Camellia::key_type keys{decipher_key_bytes};
    ar::Camellia camellia{keys};
    auto decipher_data_result = camellia.decrypts(std::get<1>(result));
    expect(decipher_data_result.has_value());

    benchmark::DoNotOptimize(decipher_data_result);
  }
}

BENCHMARK(decrypt_dmrsa)->Unit(benchmark::kMillisecond)->Arg(ARG_1)->Arg(ARG_2);
BENCHMARK(decrypt_rsa)->Unit(benchmark::kMillisecond)->Arg(ARG_1)->Arg(ARG_2);
BENCHMARK(decrypt_camellia)->Unit(benchmark::kMillisecond)->Arg(ARG_1)->Arg(ARG_2);
BENCHMARK(decrypt_aes)->Unit(benchmark::kMillisecond)->Arg(ARG_1)->Arg(ARG_2);
BENCHMARK(decrypt_hybrid)->Unit(benchmark::kMillisecond)->Arg(ARG_1)->Arg(ARG_2);
