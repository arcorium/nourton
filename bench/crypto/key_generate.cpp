#include <benchmark/benchmark.h>
#include <crypto/dm_rsa.h>
#include <crypto/rsa.h>
#include <crypto/camellia.h>
#include <crypto/aes.h>

static void key_creation_dmrsa(benchmark::State& state)
{
  for (auto _ : state)
  {
    ar::DMRSA rsa{};
    benchmark::DoNotOptimize(rsa);
  }
}

static void key_creation_rsa(benchmark::State& state)
{
  for (auto _ : state)
  {
    ar::RSA rsa{};
    benchmark::DoNotOptimize(rsa);
  }
}

static void key_creation_camellia(benchmark::State& state)
{
  for (auto _ : state)
  {
    ar::Camellia cam{};
    benchmark::DoNotOptimize(cam);
  }
}

static void key_creation_aes(benchmark::State& state)
{
  for (auto _ : state)
  {
    ar::AES aes{};
    benchmark::DoNotOptimize(aes);
  }
}

static void key_creation_hybrid(benchmark::State& state)
{
  for (auto _ : state)
  {
    ar::DMRSA rsa{};
    ar::Camellia camellia{};
    benchmark::DoNotOptimize(rsa);
    benchmark::DoNotOptimize(camellia);
  }
}

BENCHMARK(key_creation_dmrsa);
BENCHMARK(key_creation_rsa);
BENCHMARK(key_creation_camellia);
BENCHMARK(key_creation_aes);
BENCHMARK(key_creation_hybrid);
