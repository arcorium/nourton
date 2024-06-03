#pragma once

#include <array>
#include <vector>
#include <expected>

#include <alpaca/alpaca.h>

#include "util/types.h"

namespace ar
{
  template <typename T>
  concept serializable = requires(T t)
  {
    // { t.serialize() } noexcept -> std::same_as<Message>;
    { t.serialize() } noexcept -> std::same_as<std::vector<u8>>;
  };

  template <typename T>
  concept payload = serializable<T>;

  template <payload T>
  static std::expected<T, std::string_view> parse_body(std::span<const u8> payload) noexcept;

  struct Message
  {
    enum class Type : u8
    {
      Login,
      Register,
      StorePublicKey,
      StoreSymmetricKey,
      GetUserOnline,
      // UserOnlineResponse,
      GetUserDetails,
      // UserDetailsResponse,
      GetServerDetail,
      // ServerDetailResponse,
      SendFile,
      // ---Signal
      UserLogout,
      UserLogin,
      // ---Signal
      Feedback
    };

    enum class EncryptionType: u8
    {
      None,
      Asymmetric,
      Symmetric
    };

    struct Header
    {
      using opponent_id_type = u16; // FIX: reference into User::id_type instead

      u64 body_size;
      u16 body_filler; // filler will only be used to decrypt when the encrypted flag is on
      EncryptionType encryption;
      Type message_type;
      opponent_id_type opponent_id; // 0 = server

      [[nodiscard]] u64 real_size() const noexcept
      {
        return body_size - body_filler;
      }
    };

    constexpr static u8 header_size = sizeof(Header);

    Message()
      : header{}, body{}
    {
    }

    explicit Message(const Header& header_) noexcept
      : header{}, body{}
    {
      std::memcpy(header.data(), &header_, header_size);
    }

    explicit Message(const Header& header_, std::span<const u8> body) noexcept
      : header(), body{body.begin(), body.end()}
    {
      std::memcpy(header.data(), &header_, header_size);
    }

    // WARN: header should not be serialized using alpaca, use the underlying memory layout of
    // Header struct instead
    std::array<u8, header_size> header;
    std::vector<u8> body;

    [[nodiscard]]
    Header parse_header() const noexcept
    {
      Header temp{};
      std::memcpy(&temp, header.data(), header_size);
      return temp;
    }

    [[nodiscard]] Header const* as_header() const noexcept
    {
      return reinterpret_cast<Header const*>(header.data());
    }

    [[nodiscard]] Header* as_header() noexcept
    {
      return reinterpret_cast<Header*>(header.data());
    }

    template <typename T>
    [[nodiscard]] std::expected<T, std::string_view> body_as() const noexcept
    {
      return parse_body<T>(body);
    }

    [[nodiscard]] usize size() const noexcept
    {
      return header_size + body.size();
    }
  };

  template <payload T>
  std::expected<T, std::string_view> parse_body(std::span<const u8> payload) noexcept
  {
    std::error_code ec;
    auto result = alpaca::deserialize<T>(payload, ec);
    if (ec)
      return std::unexpected("failed to deserialize payload");

    return result;
  }
}