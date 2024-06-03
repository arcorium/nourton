// #define ASIO_NO_DEPRECATED
#include "application.h"

#include <GLFW/glfw3.h>

#include <argparse/argparse.hpp>
#include <asio.hpp>
#include <filesystem>

#include "application.h"
#include "core.h"
#include "logger.h"

void cli(argparse::ArgumentParser& program, int argc, char* argv[]) noexcept
{
  program.add_argument("-i", "--ip").help("ip of remote server").default_value("127.0.0.1");
  program.add_argument("-p", "--port")
      .help("port of remove server")
      .scan<'u', u16>()
      .default_value(1291_u16);
  program.add_argument("-d", "--dir")
      .help("location to save the received file")
      .default_value((std::filesystem::current_path() / "files").string());

  program.parse_args(argc, argv);
}

int main(int argc, char* argv[])
{
  auto res = glfwInit();
  if (!res)
    return -1;

  ar::Logger::set_current_thread_name("MAIN");
  ar::Logger::set_minimum_level(ar::Logger::Level::Trace);

  if constexpr (!AR_DEBUG)
  {
    ar::Logger::set_minimum_level(ar::Logger::Level::Info);
  }

  argparse::ArgumentParser program{std::string{PROGRAM_CLIENT_NAME}, std::string{PROGRAM_VERSION}};
  cli(program, argc, argv);

  asio::io_context context{};
  // HACK: prevent io to stop when there is no async action
  auto guard = asio::make_work_guard(context);

  ar::Window window{"nourton client", 800, 800};

  auto ip_str = program.get<std::string>("-i");
  auto save_dir = program.get<std::string>("-d");
  auto port = program.get<u16>("-p");

  // check if ip provided valid
  asio::error_code ec;
  asio::ip::make_address_v4(ip_str, ec);
  if (ec)
    ar::Logger::critical(fmt::format("could not connect to ip: {}", ip_str));

  ar::Application app{context, std::move(window), ip_str, port, save_dir};
  if (!app.init())
  {
    ar::Logger::critical("failed to initialize application");
    context.stop();
    return -1;
  }

  std::thread io_thread{[&] {
    ar::Logger::set_current_thread_name("IO");
    context.run();
  }};

  app.start();
  io_thread.join();
  return 0;
}
