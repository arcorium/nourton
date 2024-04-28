#pragma once

#include <string_view>

#include <imgui.h>

#include <stb_image.h>

#include <util/make.h>

namespace ar
{
    // inline bool add_font(ImGuiIO& io, std::string_view filepath, float size) noexcept
    // {
    //     auto font = io.Fonts->AddFontFromFileTTF(filepath.data(), size);
    //     return font != nullptr;
    // }
    //
    // // @copyright: https://github.com/ocornut/imgui/issues/1901#issuecomment-444929973
    // template <std::floating_point... Args>
    // bool add_fonts(ImGuiIO& io, std::string_view filepath, Args&&... size) noexcept
    // {
    //     return (add_font(io, filepath, size) || ...);
    // }

    static void center_text(std::string_view text) noexcept
    {
        auto item_size = ImGui::GetItemRectSize();
        auto text_size = ImGui::CalcTextSize(text.data());

        ImGui::SetCursorPosX((item_size.x - text_size.x) * 0.5f);
        ImGui::Text(text.data());
    }

    static void center_text_unformatted(std::string_view text) noexcept
    {
        auto item_size = ImGui::GetItemRectSize();
        auto text_size = ImGui::CalcTextSize(text.data());

        ImGui::SetCursorPosX((item_size.x - text_size.x) * 0.5f);
        ImGui::TextUnformatted(text.data());
    }

    constexpr static ImVec4 color_from_hex(u32 hex_color) noexcept
    {
        const u8 a = hex_color & 0xFF;
        const u8 b = (hex_color >> 8) & 0xFF;
        const u8 g = (hex_color >> 16) & 0xFF;
        const u8 r = (hex_color >> 24) & 0xFF;

        return ImVec4{
            static_cast<float>(r) / 255.f,
            static_cast<float>(g) / 255.f,
            static_cast<float>(b) / 255.f,
            static_cast<float>(a) / 255.f
        };
    }
}
