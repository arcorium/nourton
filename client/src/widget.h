#pragma once

#include <string_view>

#include "imgui/imgui_internal.h"

namespace ar
{
  struct InputTextCallback_UserData
  {
    std::string& Str;
    ImGuiInputTextCallback ChainCallback;
    void* ChainCallbackUserData;
  };

  static void loading_widget(const char* label, const float indicator_radius,
                             const ImVec4& main_color, const ImVec4& backdrop_color,
                             const int circle_count, const float speed) noexcept
  {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
      return;

    ImGuiContext& g = *ImGui::GetCurrentContext();
    const ImGuiID id = window->GetID(label);

    const ImVec2 pos = window->DC.CursorPos;
    const float circle_radius = indicator_radius / 15.0f;
    const float updated_indicator_radius = indicator_radius - 4.0f * circle_radius;
    const ImRect bb(pos, ImVec2(pos.x + indicator_radius * 2.0f, pos.y + indicator_radius * 2.0f));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id))
    {
      return;
    }
    const float t = g.Time;
    const auto degree_offset = 2.0f * IM_PI / circle_count;
    for (int i = 0; i < circle_count; ++i)
    {
      const auto x = updated_indicator_radius * std::sin(degree_offset * i);
      const auto y = updated_indicator_radius * std::cos(degree_offset * i);
      const auto growth = std::max(0.0f, std::sin(t * speed - i * degree_offset));
      ImVec4 color;
      color.x = main_color.x * growth + backdrop_color.x * (1.0f - growth);
      color.y = main_color.y * growth + backdrop_color.y * (1.0f - growth);
      color.z = main_color.z * growth + backdrop_color.z * (1.0f - growth);
      color.w = 1.0f;
      window->DrawList->AddCircleFilled(ImVec2(pos.x + indicator_radius + x,
                                               pos.y + indicator_radius - y),
                                        circle_radius + growth * circle_radius, ImGui::GetColorU32(color));
    }
  }

  inline int input_text_callback(ImGuiInputTextCallbackData* callback) noexcept
  {
    auto data = static_cast<InputTextCallback_UserData*>(callback->UserData);
    data->Str.resize(callback->BufTextLen);
    callback->Buf = data->Str.data();

    if (!data->ChainCallback)
      return 0;
    callback->UserData = data->ChainCallbackUserData;
    return data->ChainCallback(callback);
  }

  static bool input_text(const char* label, std::string& buffer, ImGuiInputFlags flags = 0,
                         ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr) noexcept
  {
    flags |= ImGuiInputTextFlags_CallbackResize;
    InputTextCallback_UserData cb_user_data{
      .Str = buffer,
      .ChainCallback = callback,
      .ChainCallbackUserData = user_data
    };

    return ImGui::InputText(label, buffer.data(), buffer.capacity() + 1, flags, input_text_callback,
                            &cb_user_data);
  }
}
