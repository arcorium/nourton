#pragma once

#include <util/literal.h>

#include <expected>
#include <string>

#include "util/imgui.h"

#include "core.h"

struct ImFont;

namespace ar
{
  struct ImageProperties
  {
    unsigned id;
    usize width;
    usize heigth;
  };

  // Handle resources like font and image
  class ResourceManager
  {
    template <typename Item>
    using container_type = std::unordered_map<std::string, Item>;
    using font_type = ImFont*;
    using image_type = ImageProperties;

  public:
    ResourceManager() noexcept = default;

    template <std::floating_point... Arg>
    bool load_font(std::string_view filename, Arg... sizes) noexcept;

    template <std::floating_point... Arg>
    void load_font(std::string_view filename, ImFontConfig* config, ImWchar const* ranges,
                   Arg... sizes) noexcept;

    bool load_texture(std::string_view filename) noexcept;

    font_type font(std::string_view name, float size) noexcept;
    std::expected<image_type, std::string_view> image(std::string_view name) noexcept;

  private:
#if AR_DEBUG
    constexpr static std::string_view font_dir{"../../resource/font/"sv};
    constexpr static std::string_view image_dir{"../../resource/image/"sv};
#else
    constexpr static std::string_view font_dir{"resource/font/"sv};
    constexpr static std::string_view image_dir{"resource/image/"sv};
#endif

    std::unordered_map<usize, font_type> fonts_;
    container_type<image_type> textures_;
  };

  template <std::floating_point... Arg>
  bool ResourceManager::load_font(std::string_view filename, Arg... sizes) noexcept
  {
    std::string filepath = std::string{font_dir} + filename.data();
    static auto add_font = [&](ImGuiIO& io, float size) noexcept {
      auto font = io.Fonts->AddFontFromFileTTF(filepath.data(), size);
      if (!font)
        return false;
      auto hashed = std::hash<std::string_view>{}(filename) + static_cast<usize>(size);
      fonts_.emplace(hashed, font);
      return true;
    };

    return (add_font(ImGui::GetIO(), sizes) && ...);
  }
  template <std::floating_point... Arg>
  void ResourceManager::load_font(std::string_view filename, ImFontConfig* config,
                                  ImWchar const* ranges, Arg... sizes) noexcept
  {
    std::string filepath = std::string{font_dir} + filename.data();
    static auto add_font
        = [&](ImGuiIO& io, ImFontConfig* conf, ImWchar const* range, float size) noexcept {
            auto font = io.Fonts->AddFontFromFileTTF(filepath.data(), size, conf, range);
            if (!font)
              return;
            auto hashed = std::hash<std::string_view>{}(filename) + static_cast<usize>(size);
            fonts_.emplace(hashed, font);
          };

    (add_font(ImGui::GetIO(), config, ranges, sizes), ...);
  }
}  // namespace ar
