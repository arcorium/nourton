
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
  crypto/hybrid.h
  message/message.h
  message/feedback.h
)

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

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # https://github.com/llvm/llvm-project/issues/62801
  target_compile_definitions(nourton-common PUBLIC -D__cpp_concepts=202002L -DASIO_HAS_CO_AWAIT=1)
endif ()