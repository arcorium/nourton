#pragma once

#include <string_view>

#include "imgui_internal.h"

#include "util/imgui.h"

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

  constexpr void empty_button_callback() noexcept {}

  template <std::invocable F = decltype(empty_button_callback)>
  static void notification_overlay(std::string_view id, std::string_view line_1,
                                   std::string_view line_2 = {}, F&& button_callback = empty_button_callback) noexcept
  {
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(id.data(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
      ImGui::Text(line_1.data());
      ImGui::Text(line_2.data());
      if (ImGui::Button("Close"))
      {
        std::invoke(std::forward<F>(button_callback));
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }
  }

  static void loading_overlay(bool is_close) noexcept
  {
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(State::overlay_state_id(OverlayState::Loading).data(), nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration |
                               ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove))
    {
      loading_widget("Loading...", 72.f, ImVec4{0.537f, 0.341f, 0.882f, 1.f},
                     ImVec4{0.38f, 0.24f, 0.64f, 1.f}, 14, 4.0f);
      if (is_close)
        ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
    }
  }

  //  _______________________________________________
  //  |    _______                                  |
  //  |   |       |   (1)||||||||||||||||||||||||   |
  //  |   |       |   |||||||||||||||||||||||||||   |
  //  |   |_______|                                 |
  //  |_____________________________________________|
  // 1: username
  static void user_widget(std::string_view id, std::string_view username, ResourceManager& resource_manager) noexcept
  {
    constexpr static ImVec4 GREEN_COLOR = color_from_hex(0x86ae63FF);
    constexpr static ImVec4 RED_COLOR = color_from_hex(0xe34844ff);
    static auto image_prop = resource_manager.image("profile-user_32.png").value();

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 18.f);
    if (ImGui::BeginChild(id.data(), {0.f, 40.f}, ImGuiChildFlags_Border))
    {
      ImGui::Image((ImTextureID)(intptr_t)image_prop.id, {24.f, 24.f});
      ImGui::SameLine();
      ImGui::Text(username.data());
      ImGui::SameLine(ImGui::GetWindowWidth() - 30.0f);

      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.f);

      ImGui::PushStyleColor(ImGuiCol_Button, GREEN_COLOR);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GREEN_COLOR);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, GREEN_COLOR);

      ImGui::Button(fmt::format(" ##{}", id).data(),
                    {ImGui::GetWindowHeight() * 0.6f, ImGui::GetWindowHeight() * 0.6f});

      ImGui::PopStyleColor(3);
      ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
  }

  static int selectable_user_widget(std::string_view id, std::string_view username, int i,
                                    ResourceManager& resource_manager) noexcept
  {
    static int selected_radio = 0;

    constexpr static ImVec4 GREEN_COLOR = color_from_hex(0x86ae63FF);
    constexpr static ImVec4 RED_COLOR = color_from_hex(0xe34844ff);
    static auto image_prop = resource_manager.image("profile-user_32.png").value();

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 18.f);
    if (ImGui::BeginChild(id.data(), {0.f, 46.f}, ImGuiChildFlags_Border))
    {
      ImGui::Image((ImTextureID)(intptr_t)image_prop.id, {30.f, 30.f});
      ImGui::SameLine();
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.f);
      ImGui::Text(username.data());
      ImGui::SameLine(ImGui::GetWindowWidth() - 40.0f);

      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.f);

      ImGui::PushStyleColor(ImGuiCol_CheckMark, color_from_hex(0xffffffff));
      ImGui::PushStyleColor(ImGuiCol_FrameBg, color_from_hex(0x111111ff));

      if (ImGui::RadioButton(fmt::format("##userradio{}", i).data(), &selected_radio, i))
        Logger::trace(fmt::format("Selected Radio: {}", i));

      ImGui::PopStyleColor(2);
      ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    return selected_radio;
  }

  //  _______________________________________________________________
  //  |    _______                                                  |
  //  |   |       |   (0)                                           |
  //  |   |       |   |||||||||||||||||||||||||||                   |
  //  |   |_______|                            (1)|||||||| (2)||||| |
  //  |_____________________________________________________________|
  // 0 = filename
  // 1 = sender
  // 2 = timestamp
  static void file_widget(const FileProperty& property, ResourceManager& resource_manager) noexcept
  {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.f);
    if (ImGui::BeginChild(property.filename.data(), {0.f, 64.f}, ImGuiChildFlags_Border))
    {
      ImageProperties image{};
      switch (property.format)
      {
      case FileFormat::None:
        image = resource_manager.image("none.png").value();
        break;
      case FileFormat::Image:
        image = resource_manager.image("picture.png").value();
        break;
      case FileFormat::Archive:
        image = resource_manager.image("zip.png").value();
        break;
      case FileFormat::Document:
        image = resource_manager.image("docs.png").value();
        break;
      case FileFormat::Other:
        image = resource_manager.image("other.png").value();
        break;
      }
      ImGui::Image((ImTextureID)(intptr_t)image.id, {48.f, 48.f});
      auto image_size = ImGui::GetItemRectSize();
      ImGui::SameLine(0.f, 10.f);
      ImGui::SetCursorPosY((image_size.y - 10.f) * 0.5f);
      ImGui::TextUnformatted(property.filename.data());

      auto font = resource_manager.font("FiraCodeNerdFont-SemiBold.ttf", 14.f);
      ImGui::PushFont(font);

      constexpr std::string_view sender{"Sender"};
      constexpr std::string_view time{"12-12-2004 12:45"};
      auto text_size = ImGui::CalcTextSize(sender.data());
      auto time_size = ImGui::CalcTextSize(time.data());
      auto& style = ImGui::GetStyle();

      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_Button));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_Button));

      ImGui::SameLine(ImGui::GetWindowWidth() - text_size.x - time_size.x - style.WindowPadding.x * 4.f);
      ImGui::SetCursorPosY(image_size.y - style.WindowPadding.y - 4.f);
      ImGui::Button(sender.data());
      ImGui::SameLine();

      ImGui::SetCursorPosY(image_size.y - style.WindowPadding.y - 4.f);
      ImGui::Button(time.data());

      ImGui::PopStyleColor(2);
      ImGui::PopFont();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
  }
}
