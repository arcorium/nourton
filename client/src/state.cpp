#include "state.h"

#include <map>

#include <util/enum.h>

namespace ar
{
  std::map<OverlayState, std::string_view> State::OVERLAY_NAME_IDS = {
    std::make_pair(OverlayState::InternalError, "Internal Error"),
    std::make_pair(OverlayState::ClientDisconnected, "Client Disconnected"),
    std::make_pair(OverlayState::EmptyField, "Missing Field"),
    std::make_pair(OverlayState::PasswordDifferent, "Password doesn't match"),
    std::make_pair(OverlayState::LoginFailed, "Login Failed"),
    std::make_pair(OverlayState::RegisterFailed, "Register Failed"),
    std::make_pair(OverlayState::SendFile, "Send File##2"),
    std::make_pair(OverlayState::SendFileFailed, "Send File Failed"),
    std::make_pair(OverlayState::FileNotExist, "File Doesn't Exist"),
  };

  State::State(PageState state, OverlayState overlay_state) noexcept
    : is_loading_{false}, page_state_{state}, overlay_state_{overlay_state}, last_overlay_state_{OverlayState::None} {}

  void State::active_page(PageState state) noexcept
  {
    page_state_ = state;
  }

  void State::active_overlay(OverlayState state) noexcept
  {
    last_overlay_state_ = state;
    overlay_state_ = state;
  }

  void State::active_loading_overlay() noexcept
  {
    is_loading_ = true;
  }

  void State::disable_loading_overlay() noexcept
  {
    is_loading_ = false;
  }

  void State::expect_operation_state(OperationState state) noexcept
  {
    expected_operation_state_ = state;
  }

  void State::operation_state(OperationState state) noexcept
  {
    current_operation_state_ = state;
  }

  void State::operation_state_complete() noexcept
  {
    current_operation_state_ = expected_operation_state_;
    expected_operation_state_ = OperationState::None;
  }

  PageState State::page_state() const noexcept
  {
    return page_state_;
  }

  OverlayState State::overlay_state() const noexcept
  {
    return last_overlay_state_;
  }

  std::string_view State::overlay_state_id(OverlayState state) noexcept
  {
    return OVERLAY_NAME_IDS[state];
  }

  OperationState State::operation_state() const noexcept
  {
    return current_operation_state_;
  }

  OperationState State::expected_operation_state() const noexcept
  {
    return expected_operation_state_;
  }

  std::string_view State::loading_overlay_id() noexcept
  {
    using namespace std::literals;
    return "Loading"sv;
  }

  OverlayState State::toggle_overlay_state() noexcept
  {
    return std::exchange(overlay_state_, OverlayState::None);
  }

  bool State::is_loading() const noexcept
  {
    return is_loading_;
  }
}
