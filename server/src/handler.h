#pragma once

#include <span>
#include "util/types.h"

namespace ar
{
  struct Message;
  class Connection;

  class IMessageHandler
  {
  public:
    virtual ~IMessageHandler() = default;
    virtual void on_message_in(Connection &conn, const Message &msg) noexcept = 0;
    virtual void on_message_out(Connection &conn, std::span<const u8> bytes) noexcept = 0;
  };

  template <typename T>
  concept message_handler = requires(T t, Connection &conn, const Message &msg)
  {
    { t.on_message_in(conn, msg) } noexcept -> std::same_as<void>;
    { t.on_message_out(conn, msg) } noexcept -> std::same_as<void>;
  };

  class IConnectionHandler
  {
  public:
    virtual ~IConnectionHandler() = default;
    virtual void on_connection_closed(Connection &conn) noexcept = 0;
  };
}
