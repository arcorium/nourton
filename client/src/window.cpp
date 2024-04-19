#include "window.h"

#include <GLFW/glfw3.h>

namespace ar
{
	Window::Window(std::string_view title, u32 width, u32 heigth) noexcept
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		m_window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(heigth), title.data(), nullptr, nullptr);
	}

	Window::~Window() noexcept
	{
		destroy();
	}

	Window::Window(Window&& other) noexcept
		: m_window(other.m_window)
	{
		other.m_window = nullptr;
	}

	Window& Window::operator=(Window&& other) noexcept
	{
		if (&other == this)
			return *this;

		m_window = other.m_window;
		other.m_window = nullptr;
		return *this;
	}

	void Window::destroy() noexcept
	{
		if (m_window)
		{
			glfwDestroyWindow(m_window);
			m_window = nullptr;
		}
	}

	void Window::update() noexcept
	{
		glfwPollEvents();
	}

	void Window::render() noexcept
	{
		glfwSwapBuffers(m_window);
	}

	bool Window::is_exit() const noexcept
	{
		return glfwWindowShouldClose(m_window);
	}

	void Window::exit() const noexcept
	{
		glfwSetWindowShouldClose(m_window, true);
	}

	std::tuple<u32, u32> Window::size() const noexcept
	{
		int width, height;
		glfwGetWindowSize(m_window, &width, &height);
		return std::make_tuple(static_cast<u32>(width), static_cast<u32>(height));
	}
}
