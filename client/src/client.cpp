//
// Created by arcorium on 4/16/2024.
//

#include "client.h"

#include <magic_enum.hpp>
#include <fmt/format.h>

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/thread_pool.hpp>
#include <utility>

#include "file.h"
#include "handler.h"
#include "logger.h"
#include "util/time.h"

#include "message/feedback.h"

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
      connection_{executor_},
      symm_encryptor_{Camellia::create()}
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
    asio::co_spawn(executor_, [this] {
      return reader();
    }, asio::detached);

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

  bool Client::send_file(const asymm_enc::_public_key& pk, std::string_view filepath,
                         User::id_type opponent_id) noexcept
  {
    std::string_view filename = get_filename_with_format(filepath);

    // Read file
    auto result = read_file_as_bytes(filepath);
    if (!result.has_value())
    {
      Logger::error(result.error());
      return false;
    }

    auto enc_result = encrypt(pk, result.value());
    auto enc_key_bytes = ar::as_byte_span<asymm_type::block_enc_type>(enc_result.cipher_key);

    // Send payload
    SendFilePayload payload{
        .file_filler = enc_result.data_padding,
        .key_filler = enc_result.key_padding,
        .file_size = result->size(),
        .filename = std::string{filename},
        .timestamp = get_current_time(),
        .symmetric_key = std::vector<u8>{enc_key_bytes.begin(), enc_key_bytes.end()},
        .files = std::move(enc_result.cipher_data)
    };

    write<false>(std::move(payload), opponent_id);
    return true;
  }

  asymm_type& Client::asymmetric_encryptor() noexcept
  {
    return asymm_encryptor_;
  }

  symm_type& Client::symmetric_encryptor() noexcept
  {
    return symm_encryptor_;
  }

  void Client::send_message(Message&& payload) noexcept
  {
    const auto header = payload.as_header();
    Logger::info(fmt::format("Client sent {} message to {}",
                             magic_enum::enum_name(header->message_type), header->opponent_id));
    connection_.write(std::forward<Message>(payload));
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
      Logger::trace("Client got message from remote");
      message_handler(msg.value());
    }

    asio::post(executor_, [this] {
      disconnect();
    });
  }

  void Client::message_handler(const Message& msg) noexcept
  {
    auto header = msg.as_header();

    Logger::info(fmt::format("Client got {} message from {}",
                             magic_enum::enum_name(header->message_type), header->opponent_id));

    switch (header->message_type)
    {
    case Message::Type::SendFile: {
      auto result = get_payload<SendFilePayload>(msg);
      if (!result)
      {
        Logger::error(fmt::format("failed to deserialize send file payload: {}", result.error()));
        break;
      }

      // Decrypt data
      auto file_result = decrypt(asymm_encryptor_, result->key_filler, result->symmetric_key,
                                 result->file_filler,
                                 result->files);

      if (!file_result)
      {
        Logger::error(fmt::format("failed to decrypt on received files payload: {}",
                                  file_result.error()));
        return;
      }

      if (event_handler_)
        event_handler_->on_file_receive(
            *header, ReceivedFile{.filename = result->filename, .files = file_result.value()});
      break;
    }
    case Message::Type::UserLogin: {
      auto result = get_payload<UserLoginPayload>(msg);
      if (!result)
      {
        Logger::error(fmt::format("failed to deserialize user login payload: {}", result.error()));
        break;
      }
      if (event_handler_)
        event_handler_->on_user_login(result.value());
      break;
    }
    case Message::Type::UserLogout: {
      auto result = get_payload<UserLogoutPayload>(msg);
      if (!result)
      {
        Logger::error(fmt::format("failed to deserialize user login payload: {}", result.error()));
        break;
      }
      if (event_handler_)
        event_handler_->on_user_logout(result.value());
      break;
    }
    case Message::Type::GetUserOnline: {
      auto result = get_payload<UserOnlinePayload>(msg);
      if (!result)
      {
        Logger::error(fmt::format("failed to deserialize user online payload: {}", result.error()));
        break;
      }
      if (event_handler_)
        event_handler_->on_user_online_response(result.value());
      break;
    }
    case Message::Type::GetUserDetails: {
      auto result = get_payload<UserDetailPayload>(msg);
      if (!result)
      {
        Logger::error(fmt::format("failed to deserialize user details payload: {}",
                                  result.error()));
        break;
      }
      if (event_handler_)
        event_handler_->on_user_detail_response(result.value());
      break;
    }
    case Message::Type::GetServerDetail: {
      auto result = get_payload<ServerDetailsPayload>(msg);
      if (!result)
      {
        Logger::error(fmt::format("failed to deserialize feedback payload: {}",
                                  result.error()));
        break;
      }
      if (event_handler_)
        event_handler_->on_server_detail_response(result.value());
      break;
    }
    case Message::Type::Feedback: {
      auto result = get_payload<FeedbackPayload>(msg);
      if (!result)
      {
        Logger::error(fmt::format("failed to deserialize feedback payload: {}",
                                  result.error()));
        break;
      }
      if (event_handler_)
        event_handler_->on_feedback_response(result.value());
      break;
    }
    }
  }
} // namespace ar