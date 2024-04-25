#pragma once

#include <string_view>

#include <imgui.h>

namespace ar {
    inline void add_font(ImGuiIO &io, std::string_view filepath, float size) noexcept {
        io.Fonts->AddFontFromFileTTF(filepath.data(), size);
    }

    // @copyright: https://github.com/ocornut/imgui/issues/1901#issuecomment-444929973
    template<std::floating_point... Args>
    void add_fonts(ImGuiIO &io, std::string_view filepath, Args &&... size) noexcept {
        (add_font(io, filepath, size), ...);
    }
}
