find_package(asio REQUIRED)
find_package(fmt REQUIRED)
#find_package(range-v3 REQUIRED)
find_package(alpaca REQUIRED)
find_package(magic_enum REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(nourton-client
	src/main.cpp
	src/client.cpp
	src/client.h
	src/application.cpp
	src/application.h
	src/window.cpp
	src/window.h
)


add_subdirectory(thirdparty/imgui)

target_link_libraries(nourton-client PUBLIC asio::asio fmt::fmt
	alpaca::alpaca glfw imgui
)