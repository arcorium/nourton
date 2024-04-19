#define ASIO_NO_DEPRECATED
#include <asio.hpp>

#include <iostream>
#include <fmt/format.h>

#include "logger.h"
#include "server.h"


int main()
{
	asio::io_context context{};

	asio::ip::tcp::endpoint ep{asio::ip::tcp::v4(), 1231};
	ar::Server server{context, ep};
	server.start();

	asio::signal_set signals{context, SIGINT, SIGABRT, SIGTERM};
	signals.async_wait([&context](const asio::error_code& ec, int signal) {
		if (ec)
		{
			ar::Logger::warn(fmt::format("Signal Set Error: {}", ec.message()));
			return;
		}
		ar::Logger::warn(fmt::format("Got signal: {}", signal));
		context.stop();
	});

	asio::detail::thread_group threads{};
	threads.create_threads([&context] { context.run(); }, 5);
	threads.join();
	return 0;
}
