#pragma once

#include <string_view>

#include "util/types.h"

struct GLFWwindow;

namespace ar
{
  class Window
  {
  public:
    Window(const std::string& title, u32 width, u32 heigth) noexcept;
    ~Window() noexcept;

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    void destroy() noexcept;
    void update() noexcept;
    void render() noexcept;

    void show() const noexcept;
    void hide() const noexcept;

    [[nodiscard]] bool is_exit() const noexcept;
    void exit() const noexcept;
    [[nodiscard]] std::tuple<u32, u32> size() const noexcept;

    template <typename Self>
    auto&& handle(this Self&& self) noexcept;

  private:
    GLFWwindow* window_;
  };

  template <typename Self>
  auto&& Window::handle(this Self&& self) noexcept
  {
    return std::forward<Self>(self).window_;
  }
}  // namespace ar
