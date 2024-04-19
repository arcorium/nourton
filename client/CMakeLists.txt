find_package(asio REQUIRED)
#find_package(range-v3 REQUIRED)

add_executable(nourton-client
	src/main.cpp
	src/client.cpp
	src/client.h
	src/application.cpp
	src/application.h
	src/window.cpp
	src/window.h
	src/connection.h
	src/connection.cpp
)


add_subdirectory(thirdparty/imgui)
add_subdirectory(thirdparty/glad)

target_link_libraries(nourton-client PUBLIC imgui nourton-common glad)