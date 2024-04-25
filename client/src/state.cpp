#include "state.h"

#include <map>

#include <util/enum.h>

namespace ar
{
  std::map<OverlayState, std::string_view> State::OVERLAY_NAME_IDS = {
    std::make_pair(OverlayState::Loading, "Loading"),
    std::make_pair(OverlayState::ClientDisconnected, "Client Disconnected"),
    std::make_pair(OverlayState::EmptyField, "Missing Field"),
    std::make_pair(OverlayState::PasswordDifferent, "Password doesn't match"),
    std::make_pair(OverlayState::LoginFailed, "Login Failed"),
    std::make_pair(OverlayState::RegisterFailed, "Register Failed"),
  };

  State::State(PageState state, OverlayState overlay_state) noexcept
    : is_loading_{overlay_state == OverlayState::Loading}, page_state_{state},
      overlay_state_{overlay_state}, show_pages_{}
  {
    show_pages_[to_underlying(state)] = true;
  }

  void State::active_page(PageState state) noexcept
  {
    show_pages_[to_underlying(page_state_)] = false;
    show_pages_[to_underlying(state)] = true;
    page_state_ = state;
  }

  void State::active_overlay(OverlayState state) noexcept
  {
    if (state == OverlayState::Loading)
      is_loading_ = true;
    else
      is_loading_ = false;
    overlay_state_ = state;
  }

  PageState State::page_state() const noexcept
  {
    return page_state_;
  }

  OverlayState State::overlay_state() const noexcept
  {
    return overlay_state_;
  }

  std::string_view State::overlay_state_id(OverlayState state) noexcept
  {
    return OVERLAY_NAME_IDS[state];
  }

  OverlayState State::toggle_overlay_state() noexcept
  {
    return std::exchange(overlay_state_, OverlayState::None);
  }

  bool* State::page_show(PageState state) noexcept
  {
    return &show_pages_[to_underlying(state)];
  }

  bool State::is_loading() const noexcept
  {
    return is_loading_;
  }
}
