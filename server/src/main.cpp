#define ASIO_NO_DEPRECATED
#include <asio.hpp>

#include <fmt/format.h>

#include "logger.h"
#include "server.h"

#include "util/asio.h"

int main()
{
	ar::Logger::set_current_thread_name("MAIN");

	constexpr static usize thread_num = 5;
	asio::thread_pool context{thread_num};

	auto guard = asio::make_work_guard(context);

	asio::ip::tcp::endpoint ep{asio::ip::tcp::v4(), 1231};
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
