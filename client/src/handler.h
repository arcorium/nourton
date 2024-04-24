#pragma once

namespace ar
{
  class IEventHandler
  {
  public:
    virtual ~IEventHandler() noexcept = default;
    virtual void on_feedback_response(const FeedbackPayload& payload) noexcept = 0;
    virtual void on_file_receive() noexcept = 0; // TODO: Implement this
  };
}
