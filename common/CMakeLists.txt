
find_package(fmt REQUIRED)
find_package(alpaca REQUIRED)
find_package(magic_enum REQUIRED)
find_package(asio REQUIRED)
find_package(Boost REQUIRED)
#find_package(Boost REQUIRED COMPONENTS multiprecision)

add_library(nourton-common STATIC
  crypto/camellia.h
  crypto/camellia.cpp
  message/payload.h
  util/types.h
  util/literal.h
  util/make.h
  util/scope_guard.h
  util/convert.h
  util/enum.h
  util/algorithm.h
  util/asio.h
  logger.h
  logger.cpp
  crypto/dm_rsa.cpp
  crypto/dm_rsa.h
  util/file.h
  util/concept.h
  util/time.h
  util/tinyfd.h
  crypto/rsa.cpp
  crypto/rsa.h
  crypto/aes.cpp
  crypto/aes.h
  core.h
)

target_compile_definitions(nourton-common PUBLIC _WIN32_WINNT=0x0A00)

target_include_directories(nourton-common PUBLIC .)

target_link_libraries(nourton-common PUBLIC
  fmt::fmt
  alpaca::alpaca
  magic_enum::magic_enum
  Boost::boost
  asio::asio
  primesieve::primesieve
  aes
)