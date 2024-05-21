//
// Created by mizzh on 4/28/2024.
//

#include "resource.h"

#include <fmt/format.h>

#include "glad/glad.h"
#include "logger.h"
#include "util/make.h"

#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

namespace ar
{
  static std::expected<ImageProperties, std::string_view> upload_texture(
      std::string_view filepath) noexcept
  {
    int x, y;
    auto image = stbi_load(filepath.data(), &x, &y, nullptr, STBI_rgb_alpha);
    // auto image2 = stbi_loadf(filepath.data(), &x, &y, &channel, STBI_rgb_alpha);
    if (!image)
      return std::unexpected("could not load image"sv);

    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    // glCreateTextures(GL_TEXTURE_2D, 1, &texture_id);

    // glTextureParameteri()
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    stbi_image_free(image);
    return ar::make_expected<ImageProperties, std::string_view>(texture_id, static_cast<usize>(x),
                                                                static_cast<usize>(y));
  }

  bool ResourceManager::load_image(std::string_view filename) noexcept
  {
    std::string filepath = std::string{image_dir} + filename.data();
    auto res = upload_texture(filepath);
    if (!res.has_value())
    {
      return false;
    }
    textures_.emplace(std::string{filename}, res.value());
    return true;
  }

  ResourceManager::font_type ResourceManager::font(std::string_view name, float size) noexcept
  {
    const auto id = std::hash<std::string_view>{}(name) + static_cast<usize>(size);
    if (!fonts_.contains(id))
    {
      // load
      if (!load_font(name, size))
      {
        Logger::error(fmt::format("failed to load font {}", name));
        return nullptr;
      }
    }
    return fonts_[id];
  }

  std::expected<ResourceManager::image_type, std::string_view> ResourceManager::image(
      std::string_view name) noexcept
  {
    if (!textures_.contains(std::string{name}))
    {
      // load
      if (!load_image(name))
        // WARN: string lifetime is on function scope, so it should return std::string instead
        ar::unexpected<std::string_view>(fmt::format("failed to load texture: {}", name));
    }
    return ar::make_expected<image_type, std::string_view>(textures_[std::string{name}]);
  }
}  // namespace ar
