#pragma once

namespace ar
{
  struct UserLoginPayload;
  struct FeedbackPayload;
  struct SendFilePayload;
  struct UserLogoutPayload;
  struct UserDetailPayload;
  struct UserOnlinePayload;

  struct ReceivedFile
  {
    std::string_view filename;
    std::span<u8> files;
  };

  class IEventHandler
  {
  public:
    virtual ~IEventHandler() noexcept = default;
    // from server
    virtual void on_feedback_response(const FeedbackPayload& payload) noexcept = 0;
    virtual void on_user_login(const UserLoginPayload& payload) noexcept = 0;
    virtual void on_user_logout(const UserLogoutPayload& payload) noexcept = 0;
    virtual void on_user_detail_response(const UserDetailPayload& payload) noexcept = 0;
    virtual void on_server_detail_response(const ServerDetailsPayload& payload) noexcept = 0;
    virtual void on_user_online_response(const UserOnlinePayload& payload) noexcept = 0;
    // from other client
    virtual void on_file_receive(const Message::Header& header,
                                 const ReceivedFile& received_file) noexcept
    = 0;
  };
} // namespace ar