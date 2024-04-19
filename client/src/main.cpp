#define ASIO_NO_DEPRECATED
#include <asio.hpp>

#include <GLFW/glfw3.h>
#include <imgui/imgui.h>

#include "application.h"
#include "logger.h"

namespace gui = ImGui;

int main()
{
	auto res = glfwInit();
	if (!res)
		return -1;
	asio::thread_pool context{2};
	ar::Window window{"nourton client"sv, 800, 800};
	ar::Application app{context, std::move(window)};
	if (!app.init())
	{
		ar::Logger::critical("failed to initialize application");
		context.stop();
		context.join();
		return -1;
	}

	app.start();
	context.join();
	return 0;
}
