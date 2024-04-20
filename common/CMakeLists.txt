
find_package(fmt REQUIRED)
find_package(alpaca REQUIRED)
find_package(magic_enum REQUIRED)
find_package(asio REQUIRED)

add_library(nourton-common STATIC
	message/payload.h
	util/types.h
	util/literal.h
	util/make.h
	util/scope_guard.h
	logger.h
	logger.cpp
)

target_compile_definitions(nourton-common PUBLIC _WIN32_WINNT=0x0A00)

target_include_directories(nourton-common INTERFACE .)

target_link_libraries(nourton-common PUBLIC fmt::fmt alpaca::alpaca magic_enum::magic_enum asio::asio)