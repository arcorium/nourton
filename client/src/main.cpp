#include <asio.hpp>

#include <imgui/imgui.h>
#include <GLFW/glfw3.h>

namespace gui = ImGui;

int main()
{
  asio::io_context context{};
  asio::error_code ec;
  auto address = asio::ip::make_address_v4("127.0.0.1", ec);
  if (!ec)
    return 0;

  asio::ip::tcp::endpoint endpoint{address, 1234};
  asio::ip::tcp::socket socket{context};
  socket.async_connect(endpoint, )

  if (!glfwInit())
  {
    return 0;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  auto window = glfwCreateWindow(800, 800, "mizhan", nullptr, nullptr);
  if (!window)
  {
    return 0;
  }

  IMGUI_CHECKVERSION();
  gui::CreateContext();
  auto& io = gui::GetIO();
  auto& style = gui::GetStyle();

  gui::StyleColorsDark(&style);

  asio::detail::thread_group threads{};
  threads.create_threads([&context] { context.run(); }, 5);

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
  }

  threads.join();

  return 0;
}
