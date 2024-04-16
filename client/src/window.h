#pragma once

#include <string>
#include <GLFW/glfw3.h>

#include <util>

namespace ar
{
  class Window
  {
  public:
    Window(std::string_view title, u32 width, u32 heigth)

  private:
    GLFWwindow* m_window;
  };
}
