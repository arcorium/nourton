


find_package(asio REQUIRED)
find_package(fmt REQUIRED)
find_package(range-v3 REQUIRED)
find_package(alpaca REQUIRED)

add_executable(server src/main.cpp
        src/util/scope_guard.h
        src/server.cpp
        src/server.h
        src/connection.cpp
        src/connection.h
        src/message/payload.h
        src/util/literal.h
        src/util/types.h
        src/logger.cpp
        src/logger.h
        src/util/make.h
        src/handler.h
        src/user.cpp
        src/user.h
        src/generator.h
)

target_link_libraries(server PUBLIC asio::asio fmt::fmt range-v3::meta range-v3::concepts range-v3::range-v3 alpaca::alpaca)