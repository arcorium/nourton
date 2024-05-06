#pragma once

#include <asio/use_awaitable.hpp>
#include <asio/as_tuple.hpp>
#include <asio/redirect_error.hpp>

namespace ar
{
  inline auto await_with_error(asio::error_code& ec) noexcept
  {
    return asio::redirect_error(asio::use_awaitable, ec);
  }

  inline auto await_with_error() noexcept
  {
    return asio::as_tuple(asio::use_awaitable);
  }

  inline bool is_connection_lost(const asio::error_code& ec) noexcept
  {
    return ec == asio::error::connection_aborted || ec == asio::error::connection_refused
           || ec == asio::error::connection_reset || ec == asio::error::operation_aborted
           || ec == asio::error::eof;
  }
}  // namespace ar
