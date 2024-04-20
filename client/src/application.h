#pragma once
#include "client.h"
#include "window.h"

#include "state.h"

namespace asio
{
  class thread_pool;
}

namespace ar
{
  class Application
  {
  public:
    Application(asio::io_context& ctx, Window window) noexcept;
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
    asio::io_context& m_context;
    std::atomic_bool m_is_ready;
    State m_gui_state;

    std::string m_username;
    std::string m_password;

    Window m_window;
    Client m_client;
  };
}
