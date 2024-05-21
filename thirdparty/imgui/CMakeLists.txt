
find_package(glfw3 REQUIRED)

add_library(imgui STATIC
	imconfig.h
	imgui.h
	imgui.cpp
	imgui_demo.cpp
	imgui_draw.cpp
	imgui_internal.h
	imgui_tables.cpp
	imgui_widgets.cpp
	imstb_rectpack.h
	imstb_textedit.h
	imstb_truetype.h
	backends/imgui_impl_glfw.h
	backends/imgui_impl_glfw.cpp
	backends/imgui_impl_opengl3.h
	backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC .)

target_link_libraries(imgui PUBLIC glfw)