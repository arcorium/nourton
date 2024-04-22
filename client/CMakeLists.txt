
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

target_link_libraries(nourton-client PUBLIC imgui nourton-common glad)