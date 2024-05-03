#find_package(range-v3 REQUIRED)
find_package(argparse REQUIRED)

add_executable(nourton-server
  src/main.cpp
  src/server.cpp
  src/server.h
  src/handler.h
  src/generator.h
  src/connection.h
  src/connection.cpp
)

target_link_libraries(nourton-server PRIVATE nourton-common argparse::argparse)