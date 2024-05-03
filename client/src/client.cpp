//
// Created by arcorium on 4/16/2024.
//

#include "client.h"

#include <fmt/format.h>

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/thread_pool.hpp>
#include <utility>

#include "handler.h"
#include "logger.h"

namespace ar
{
  Client::Client(asio::any_io_executor executor, asio::ip::address address, u16 port,
                 IEventHandler* event_handler) noexcept
      : Client(std::move(executor), asio::ip::tcp::endpoint{address, port}, event_handler)
  {
  }

  Client::Client(asio::any_io_executor executor, asio::ip::tcp::endpoint endpoint,
                 IEventHandler* event_handler) noexcept
      : event_handler_{event_handler},
        executor_{std::move(executor)},
        endpoint_{std::move(endpoint)},
        connection_{executor_}
  {
    Logger::info(fmt::format("client connecting to {}: {}", endpoint_.address().to_string(),
                             endpoint_.port()));
  }

  Client::~Client() noexcept = default;

  Client::Client(Client&& other) noexcept
      : event_handler_{other.event_handler_},
        executor_{std::move(other.executor_)},
        endpoint_{std::move(other.endpoint_)},
        connection_{std::move(other.connection_)}
  {
    other.event_handler_ = nullptr;
  }

  asio::awaitable<bool> Client::start() noexcept
  {
    Logger::trace("Application trying to connect...");
    auto ec = co_await connection_.connect(endpoint_);
    if (ec)
    {
      Logger::error(fmt::format("failed to connect into endpoint : {}", ec.message()));
      co_return false;
    }

    Logger::info("Application connected to endpoint");

    // writer coroutine
    connection_.start();
    // reader coroutine
    asio::co_spawn(executor_, [this] { return reader(); }, asio::detached);

    co_return true;
  }

  void Client::disconnect() noexcept
  {
    connection_.close();
  }

  bool Client::is_connected() const noexcept
  {
    return connection_.is_open();
  }

  asio::awaitable<void> Client::reader() noexcept
  {
    Logger::trace("Client start reading data from remote");
    while (connection_.is_open())
    {
      auto msg = co_await connection_.read();
      if (!msg.has_value())
      {
        Logger::warn(fmt::format("Failed to read data: {}", msg.error().message()));
        break;
      }
      Logger::info("Client got message from remote");
      message_handler(msg.value());
    }

    asio::post(executor_, [this] { disconnect(); });
  }

  void Client::message_handler(const Message& msg) noexcept
  {
    Logger::trace("Client handle incoming message");

    auto header = msg.as_header();

    switch (header->message_type)
    {
    // TODO: Implement this
    case Message::Type::SendFile:
      if (event_handler_)
        event_handler_->on_file_receive(*header, msg.body_as<SendFilePayload>());
      break;
    case Message::Type::UserLogin:
      if (event_handler_)
        event_handler_->on_user_login(msg.body_as<UserLoginPayload>());
      break;
    case Message::Type::UserLogout:
      if (event_handler_)
        event_handler_->on_user_logout(msg.body_as<UserLogoutPayload>());
      break;
    case Message::Type::UserOnlineResponse:
      if (event_handler_)
        event_handler_->on_user_online_response(msg.body_as<UserOnlinePayload>());
      break;
    case Message::Type::UserDetailsResponse:
      if (event_handler_)
        event_handler_->on_user_detail_response(msg.body_as<UserDetailPayload>());
      break;
    case Message::Type::Feedback:
      if (event_handler_)
        event_handler_->on_feedback_response(msg.body_as<FeedbackPayload>());
      break;
    }
  }
}  // namespace ar
