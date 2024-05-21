#include <benchmark/benchmark.h>

#include "util/convert.h"
#include "util/file.h"

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

static void encrypt_dmrsa(benchmark::State& state)
{
  std::vector<u64> bytes{};
  auto val = state.range();
  bytes.resize(MB * val / sizeof(u64));
  std::ranges::fill(bytes, 0x20);

  for (auto _ : state)
  {
    auto bytes_byte = ar::as_byte_span<u64>(bytes);
    auto result = dmrsa.encrypts(bytes_byte);
    benchmark::DoNotOptimize(result);
  }
}

static ar::RSA rsa{};

static void encrypt_rsa(benchmark::State& state)
{
  std::vector<u64> bytes{};
  auto val = state.range();
  bytes.resize(MB * val / sizeof(u64));
  std::ranges::fill(bytes, 0x20);

  for (auto _ : state)
  {
    auto bytes_byte = ar::as_byte_span<u64>(bytes);
    auto result = rsa.encrypts(bytes_byte);
    benchmark::DoNotOptimize(result);
  }
}

static ar::Camellia camellia{};

static void encrypt_camellia(benchmark::State& state)
{
  std::vector<u64> bytes{};
  auto val = state.range();
  bytes.resize(MB * val / sizeof(u64));
  std::ranges::fill(bytes, 0x20);

  for (auto _ : state)
  {
    auto bytes_byte = ar::as_byte_span<u64>(bytes);
    auto result = camellia.encrypts(bytes_byte);
    benchmark::DoNotOptimize(result);
  }
}

ar::AES aes{};

static void encrypt_aes(benchmark::State& state)
{
  std::vector<u64> bytes{};
  auto val = state.range();
  bytes.resize(MB * val / sizeof(u64));
  std::ranges::fill(bytes, 0x20);

  for (auto _ : state)
  {
    auto bytes_byte = ar::as_byte_span<u64>(bytes);
    auto result = aes.encrypts(bytes_byte);
    benchmark::DoNotOptimize(result);
  }
}

static void encyrpt_hybrid(benchmark::State& state)
{
  std::vector<u64> data_bytes{};
  auto val = state.range();
  data_bytes.resize(MB * val / sizeof(u64));
  std::ranges::fill(data_bytes, 0x20);

  auto key_bytes = camellia.key();
  std::vector<u8> key{key_bytes.begin(), key_bytes.end()};
  for (auto _ : state)
  {
    // Encrypt key
    auto result_key = dmrsa.encrypts(key);
    // Encrypt data
    auto bytes_byte = ar::as_byte_span<u64>(data_bytes);
    auto result = camellia.encrypts(bytes_byte);
    benchmark::DoNotOptimize(result);
    benchmark::DoNotOptimize(result_key);
  }
}

BENCHMARK(encrypt_dmrsa)->Unit(benchmark::kMillisecond)->Arg(ARG_1)->Arg(ARG_2);
BENCHMARK(encrypt_rsa)->Unit(benchmark::kMillisecond)->Arg(ARG_1)->Arg(ARG_2);
BENCHMARK(encrypt_camellia)->Unit(benchmark::kMillisecond)->Arg(ARG_1)->Arg(ARG_2);
BENCHMARK(encrypt_aes)->Unit(benchmark::kMillisecond)->Arg(ARG_1)->Arg(ARG_2);
BENCHMARK(encyrpt_hybrid)->Unit(benchmark::kMillisecond)->Arg(ARG_1)->Arg(ARG_2);

