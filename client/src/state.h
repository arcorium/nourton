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
  };

  enum class OverlayState : u8
  {
    None,
    InternalError,
    ClientDisconnected,
    EmptyField,
    PasswordDifferent,
    LoginFailed,
    RegisterFailed,
    SendFile,
    SendFileFailed,
    FileNotExist,
  };

  enum class OperationState: u8
  {
    None,
    Login,
    Register,
    StorePublicKey,
    GetUserOnline,
    SendFile,
    GetUserDetails,
    EncryptFile,
    ReceivingFile,
    DecryptFile,
  };

  class State
  {
  public:
    explicit State(PageState state, OverlayState overlay_state = OverlayState::None) noexcept;

    void active_page(PageState state) noexcept;

    void active_overlay(OverlayState state) noexcept;

    void active_loading_overlay() noexcept;
    void disable_loading_overlay() noexcept;

    void expect_operation_state(OperationState state) noexcept;
    void operation_state(OperationState state) noexcept;
    // set current operation state into expected one and make the expected one into none
    void operation_state_complete() noexcept;

    [[nodiscard]] PageState page_state() const noexcept;
    [[nodiscard]] OverlayState overlay_state() const noexcept;

    [[nodiscard]] static std::string_view loading_overlay_id() noexcept;
    [[nodiscard]] static std::string_view overlay_state_id(OverlayState state) noexcept;

    [[nodiscard]] OperationState operation_state() const noexcept;
    [[nodiscard]] OperationState expected_operation_state() const noexcept;

    /**
     * get current overlay state and set it into OverlayState::None afterward indirectly
     * @return current active overlay state
     */
    [[nodiscard]] OverlayState toggle_overlay_state() noexcept;

    [[nodiscard]] bool is_loading() const noexcept;

  private:
    static std::map<OverlayState, std::string_view> OVERLAY_NAME_IDS;
    bool is_loading_;

    // State for UI pages
    PageState page_state_;
    // State for UI overlay
    OverlayState overlay_state_;
    OverlayState last_overlay_state_;
    // State for operation
    OperationState current_operation_state_;
    OperationState expected_operation_state_; // waiting operation state
  };
}
