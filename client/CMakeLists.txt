
add_executable(nourton-client
	src/main.cpp
	src/client.cpp
	src/client.h
	src/application.h
	src/window.cpp
	src/window.h
	src/connection.h
	src/connection.cpp
	src/state.cpp
	src/util/imgui.h
	src/handler.h
	src/widget.h
	src/application.cpp
)

target_link_libraries(nourton-client PUBLIC imgui nourton-common glad icon_font)