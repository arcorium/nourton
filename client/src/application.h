#pragma once
#include "client.h"
#include "window.h"

namespace ar
{
  enum class PageState : u8
  {
    Login,
    Register,
    Dashboard,
    SendFile,
    End
  };

  template <typename T> requires std::is_scoped_enum_v<T>
  static constexpr std::underlying_type_t<T> to_underlying(T val) noexcept
  {
    return static_cast<std::underlying_type_t<T>>(val);
  }

  class State
  {
  public:
    State(PageState state) noexcept
      : m_state{state}, m_show_page{}
    {
      m_show_page[to_underlying(state)] = true;
    }

    void active(PageState state) noexcept
    {
      m_show_page[to_underlying(m_state)] = false;
      m_show_page[to_underlying(state)] = true;
      m_state = state;
    }

    PageState state() const noexcept
    {
      return m_state;
    }

    bool* page_show(PageState state) noexcept
    {
      return &m_show_page[to_underlying(state)];
    }

  private:
    PageState m_state;
    bool m_show_page[static_cast<int>(PageState::SendFile) + 1];
  };

  class Application
  {
    using context_type = Client::context_type;

  public:
    Application(context_type& ctx, Window window) noexcept;
    ~Application() noexcept;

    bool init() noexcept;

    void start() noexcept;

  private:
    void draw() noexcept;

    void login_page() noexcept;
    void register_page() noexcept;
    void dashboard_page() noexcept;
    void send_file_page() noexcept;

    void register_button_handler() noexcept;
    void login_button_handler() noexcept;

  private:
    context_type& m_context;
    std::atomic_bool m_is_ready;
    State m_gui_state;

    std::string m_username;
    std::string m_password;

    Window m_window;
    Client m_client;
  };
}
