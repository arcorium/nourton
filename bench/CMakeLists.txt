
find_package(benchmark REQUIRED)

add_executable(nourton-bench-key
  crypto/key_generate.cpp
)

target_link_libraries(nourton-bench-key PRIVATE benchmark::benchmark benchmark::benchmark_main nourton-common)

add_executable(nourton-bench-encrypt-2
  crypto/encrypt.cpp
  crypto/util.h
)

target_link_libraries(nourton-bench-encrypt-2 PRIVATE benchmark::benchmark benchmark::benchmark_main nourton-common)

add_executable(nourton-bench-decrypt-2
  crypto/decrypt.cpp
  crypto/util.h
)

target_link_libraries(nourton-bench-decrypt-2 PRIVATE benchmark::benchmark benchmark::benchmark_main nourton-common)

add_executable(nourton-bench-all-2
  crypto/all.cpp
  crypto/util.h
)

target_link_libraries(nourton-bench-all-2 PRIVATE benchmark::benchmark benchmark::benchmark_main nourton-common)
