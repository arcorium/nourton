#pragma once
#include <mutex>

#include "client.h"
#include "handler.h"
#include "resource.h"
#include "window.h"

#include "state.h"
#include "file.h"

#include "crypto/dm_rsa.h"

namespace asio
{
  class thread_pool;
}

namespace ar
{
  // TODO: Separate UI class
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
    void send_file_thread() noexcept;

    void draw() noexcept;
    void draw_overlay() noexcept;

    void login_page() noexcept;
    void register_page() noexcept;
    void dashboard_page() noexcept;
    void send_file_modal() noexcept;

    void register_button_handler() noexcept;
    void login_button_handler() noexcept;
    void send_file_button_handler() noexcept;

    void send_file_handler() noexcept;
    void login_feedback_handler(const FeedbackPayload& payload) noexcept;
    void register_feedback_handler(const FeedbackPayload& payload) noexcept;
    void send_file_feedback_handler(const FeedbackPayload& payload) noexcept;
    void get_user_details_feedback_handler(const FeedbackPayload& payload) noexcept;
    void get_user_online_feedback_handler(const FeedbackPayload& payload) noexcept;
    void store_public_key_feedback_handler(const FeedbackPayload& payload) noexcept;

    void on_file_drop(std::string_view paths) noexcept;

    // Callback, called by io thread
  public:
    void on_feedback_response(const FeedbackPayload& payload) noexcept override;
    void on_file_receive(const Message::Header& header, const SendFilePayload& payload) noexcept override;
    void on_user_login(const UserLoginPayload& payload) noexcept override;
    void on_user_logout(const UserLogoutPayload& payload) noexcept override;
    void on_user_detail_response(const UserDetailPayload& payload) noexcept override;
    void on_user_online_response(const UserOnlinePayload& payload) noexcept override;

  private:
    struct SendFileOperationData
    {
      User::id_type user_id;
      std::string filepath; // fullpath
    };

  private:
    bool is_running_;

    asio::io_context& context_;
    State state_;
    Window window_;

    // Encrypt data
    DMRSA asymmetric_encryptor_;
    std::thread send_file_thread_;
    std::condition_variable send_file_cv_;
    std::mutex send_file_data_mutex_;
    std::queue<SendFileOperationData> send_file_datas_;

    // UI data
    std::string username_;
    std::string password_;
    std::string confirm_password_;

    ResourceManager resource_manager_;

    // TODO: Make safe_object<T> to wrap both mutex with the actual data
    std::mutex file_mutex_;
    std::vector<FileProperty> files_;
    std::mutex user_mutex_;
    // better to use list instead, because when the vector grows it will invalidate all files that have reference on the users
    // HACK: for now the vector will reserve big number (1024)
    std::vector<UserClient> users_;
    std::unique_ptr<UserClient> this_user_;

    // Send File data
    std::string dropped_file_path_;
    int selected_user_;

    Client client_;
  };
}
