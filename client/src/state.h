#pragma once

#include <util/types.h>

#include <map>

namespace ar
{
  enum class PageState : u8
  {
    Login,
    Register,
    Dashboard,
    SendFile
  };

  enum class OverlayState : u8
  {
    None,
    Loading,
    ClientDisconnected,
    EmptyField,
    PasswordDifferent,
    LoginFailed,
    RegisterFailed,
  };

  class State
  {
  public:
    explicit State(PageState state, OverlayState overlay_state = OverlayState::None) noexcept;

    void active_page(PageState state) noexcept;

    void active_overlay(OverlayState state) noexcept;

    [[nodiscard]] PageState page_state() const noexcept;

    [[nodiscard]] OverlayState overlay_state() const noexcept;

    [[nodiscard]] static std::string_view overlay_state_id(OverlayState state) noexcept;

    /**
     * get current overlay state and set it into OverlayState::None afterward indirectly
     * @return current active overlay state
     */
    [[nodiscard]] OverlayState toggle_overlay_state() noexcept;

    bool* page_show(PageState state) noexcept;

    [[nodiscard]] bool is_loading() const noexcept;

  private:
    static std::map<OverlayState, std::string_view> OVERLAY_NAME_IDS;
    bool is_loading_;

    PageState page_state_;
    OverlayState overlay_state_;
    bool show_pages_[static_cast<int>(PageState::SendFile) + 1];
  };
}
