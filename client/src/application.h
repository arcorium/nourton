#pragma once
#include "client.h"
#include "handler.h"
#include "resource.h"
#include "window.h"

#include "state.h"
#include "file.h"

namespace asio
{
  class thread_pool;
}

namespace ar
{
  class Application : public IEventHandler
  {
  public:
    Application(asio::io_context& ctx, Window window) noexcept;
    ~Application() noexcept;

    bool init() noexcept;

    void update() noexcept;
    void start() noexcept;
    void render() noexcept;

    bool is_running() const noexcept;
    void exit() noexcept;

  private:
    void draw() noexcept;
    void draw_overlay() noexcept;

    void login_page() noexcept;
    void register_page() noexcept;
    void dashboard_page() noexcept;
    void send_file_modal() noexcept;

    void register_button_handler() noexcept;
    void login_button_handler() noexcept;
    void send_file_button_handler() noexcept;

    void send_file() noexcept;

    void on_file_drop(std::string_view paths) noexcept;

    // Callback, called by io thread
  public:
    void on_feedback_response(const FeedbackPayload& payload) noexcept override;
    void on_file_receive() noexcept override;

  private:
    bool is_running_;

    asio::io_context& context_;
    State gui_state_;
    Window window_;

    // UI data
    std::string username_;
    std::string password_;
    std::string confirm_password_;

    ResourceManager resource_manager_;

    std::vector<FileProperty> files_;
    std::vector<User> users_;

    // Send File data
    std::string dropped_file_path_;
    int selected_user_;

    Client client_;
  };
}
