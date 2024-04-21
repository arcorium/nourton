#define ASIO_NO_DEPRECATED
#include <asio.hpp>

#include <GLFW/glfw3.h>
#include <imgui/imgui.h>

#include "application.h"
#include "logger.h"

#include "crypto/camellia.h"

namespace gui = ImGui;

int main()
{
	auto res = glfwInit();
	if (!res)
		return -1;

	asio::io_context context{};
	// HACK: prevent io to stop when there is no async action
	auto guard = asio::make_work_guard(context);

	ar::Logger::set_current_thread_name("MAIN");

	ar::Window window{"nourton client"sv, 800, 800};
	ar::Application app{context, std::move(window)};
	if (!app.init())
	{
		ar::Logger::critical("failed to initialize application");
		context.stop();
		return -1;
	}

	std::thread io_thread{
		[&] {
			ar::Logger::set_current_thread_name("WORKER");
			context.run();
		}
	};
	// std::thread io_thread{&asio::io_context::run, &context};

	app.start();
	io_thread.join();
	return 0;
}
