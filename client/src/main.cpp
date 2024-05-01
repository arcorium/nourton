//#define ASIO_NO_DEPRECATED
#include <asio.hpp>
#include <filesystem>

#include <GLFW/glfw3.h>

#include "application.h"
#include "logger.h"

#include <fmt/chrono.h>

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
      ar::Logger::set_current_thread_name("IO");
      context.run();
    }
  };

  app.start();
  io_thread.join();
  return 0;
}
