#include "window.h"

#include <GLFW/glfw3.h>

namespace ar
{
	Window::Window(std::string_view title, u32 width, u32 heigth) noexcept
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		glfwWindowHint(GLFW_VISIBLE, 0);
		// glfwWindowHint(GLFW_SAMPLES, 8);
		window_ = glfwCreateWindow(static_cast<int>(width), static_cast<int>(heigth), title.data(), nullptr, nullptr);
	}

	Window::~Window() noexcept
	{
		destroy();
	}

	Window::Window(Window&& other) noexcept
		: window_(other.window_)
	{
		other.window_ = nullptr;
	}

	Window& Window::operator=(Window&& other) noexcept
	{
		if (&other == this)
			return *this;

		window_ = other.window_;
		other.window_ = nullptr;
		return *this;
	}

	void Window::destroy() noexcept
	{
		if (window_)
		{
			glfwDestroyWindow(window_);
			window_ = nullptr;
		}
	}

	void Window::update() noexcept
	{
		glfwPollEvents();
	}

	void Window::render() noexcept
	{
		glfwSwapBuffers(window_);
	}

	void Window::show() const noexcept
	{
		glfwShowWindow(window_);
	}

	void Window::hide() const noexcept
	{
		glfwHideWindow(window_);
	}

	bool Window::is_exit() const noexcept
	{
		return glfwWindowShouldClose(window_);
	}

	void Window::exit() const noexcept
	{
		glfwSetWindowShouldClose(window_, true);
	}

	std::tuple<u32, u32> Window::size() const noexcept
	{
		int width, height;
		glfwGetWindowSize(window_, &width, &height);
		return std::make_tuple(static_cast<u32>(width), static_cast<u32>(height));
	}
}
