
find_package(benchmark REQUIRED)

add_executable(nourton-bench-key
  crypto/key_generate.cpp
)

target_link_libraries(nourton-bench-key PRIVATE benchmark::benchmark benchmark::benchmark_main nourton-common)

add_executable(nourton-bench-encrypt
  crypto/encrypt.cpp
)

target_link_libraries(nourton-bench-encrypt PRIVATE benchmark::benchmark benchmark::benchmark_main nourton-common)

add_executable(nourton-bench-decrypt
  crypto/decrypt.cpp
)

target_link_libraries(nourton-bench-decrypt PRIVATE benchmark::benchmark benchmark::benchmark_main nourton-common)

add_executable(nourton-bench-all
  crypto/all.cpp
)

target_link_libraries(nourton-bench-all PRIVATE benchmark::benchmark benchmark::benchmark_main nourton-common)
