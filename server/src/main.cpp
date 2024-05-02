#define ASIO_NO_DEPRECATED
#include <asio.hpp>

#include <argparse/argparse.hpp>

#include <fmt/format.h>

#include "core.h"
#include "logger.h"
#include "server.h"

#include "util/asio.h"

void cli(argparse::ArgumentParser& program, int argc, char* argv[]) noexcept
{
	program.add_argument("-i", "--ip")
	       .help("ip of remote server")
	       .default_value("0.0.0.0");
	// .required();
	program.add_argument("-p", "--port")
	       .help("port of remove server")
	       .scan<'u', u16>()
	       .default_value(1291_u16);

	program.parse_args(argc, argv);
}

int main(int argc, char* argv[])
{
	ar::Logger::set_current_thread_name("MAIN");
	if constexpr (!AR_DEBUG)
	{
		ar::Logger::set_minimum_level(ar::Logger::Level::Info);
	}

	constexpr static usize thread_num = 5;
	asio::thread_pool context{thread_num};

	auto guard = asio::make_work_guard(context);

	// CLI
	argparse::ArgumentParser program{std::string{PROGRAM_SERVER_NAME}, std::string{PROGRAM_VERSION}};
	cli(program, argc, argv);

	auto ip_str = program.get<std::string>("-i");
	auto port = program.get<u16>("-p");

	asio::error_code ec;
	auto ip = asio::ip::make_address_v4(ip_str, ec);
	if (ec) // exit
		ar::Logger::critical(fmt::format("could not listen to ip: {}", ip_str));

	asio::ip::tcp::endpoint ep{ip, port};
	ar::Server server{context.executor(), ep};
	server.start();

	asio::signal_set signals{context, SIGINT, SIGABRT, SIGTERM};
	signals.async_wait([&context](const asio::error_code& ec, int signal) {
		if (ec)
		{
			ar::Logger::warn(fmt::format("Signal Set Error: {}", ec.message()));
			return;
		}
		ar::Logger::warn(fmt::format("Got signal: {}", signal));
		ar::Logger::info("Stopping server!");
		context.stop();
	});

	context.join();
	return 0;
}
