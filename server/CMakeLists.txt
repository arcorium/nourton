#find_package(range-v3 REQUIRED)

add_executable(nourton-server
	src/main.cpp
	src/server.cpp
	src/server.h
	src/handler.h
	src/generator.h
	src/connection.h
	src/connection.cpp
)

target_include_directories(nourton-server PRIVATE ../common)

target_link_libraries(nourton-server PUBLIC nourton-common
)