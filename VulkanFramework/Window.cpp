// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Window.h>
#include <VulkanFramework/DebugUtils.h>
#include <Common/Logger.h>
#include <cassert>

namespace vfs
{
	void	ProcessCursorPosCallback	(GLFWwindow* window, double xpos, double ypos);
	void	ProcessWindowResizeCallback	(GLFWwindow* window, int width, int height);

	Window::Window(const char* title, uint32_t width, uint32_t height)
	{
		assert(initGLFW() == true);
		assert(createWindow(title, width, height) == true);
		assert(initEvents() == true);
	}

	Window::Window(const char* title, float aspectRatio)
	{
		assert(initGLFW() == true);

		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		const uint32_t width  = static_cast<uint32_t>(mode->width  * aspectRatio);
		const uint32_t height = static_cast<uint32_t>(mode->height * aspectRatio);
		assert(createWindow(title, width, height) == true);

		assert(initEvents() == true);
	}

	Window::~Window()
	{
		destroyWindow();
	}
	
	void Window::destroyWindow(void)
	{
		if (_window != nullptr)
		{
			glfwDestroyWindow(_window);
		}
		glfwTerminate();
	}


	bool Window::initGLFW(void)
	{
		if (glfwInit() == GLFW_FALSE)
		{
			VFS_ERROR << "Failed to initialize GLFW";
			return false;
		}

		if (glfwVulkanSupported() == 0)
		{
			VFS_ERROR << "This device does not support Vulkan";
			return false;
		}
		
		return true;
	}

	bool Window::createWindow(const char* title, uint32_t width, uint32_t height)
	{
		_title	= title;
		_extent = glm::vec2(width, height);
		
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		_window = glfwCreateWindow(_extent.x, _extent.y, _title, nullptr, nullptr);
		if (_window == nullptr)
		{
			VFS_ERROR << "Failed to create GLFW window handle";
			return false;
		}

		return true;
	}

	bool Window::initEvents(void)
	{
		glfwSetWindowUserPointer(_window, this);
		glfwSetFramebufferSizeCallback(_window, ProcessWindowResizeCallback);
		glfwSetCursorPosCallback(_window, ProcessCursorPosCallback);
		glfwSetErrorCallback(DebugUtils::GlfwDebugCallback);
		
		return true;
	}

	VkSurfaceKHR Window::createWindowSurface(VkInstance instance)
	{
		VkSurfaceKHR surface;
		glfwCreateWindowSurface(instance, _window, nullptr, &surface);
		return surface;
	}

	void Window::processKeyInput(void)
	{
		for (uint32_t key = GLFW_KEY_SPACE; key < GLFW_KEY_LAST; ++key)
		{
			for (KeyCallback& callback : _keyCallbacks)
			{
				callback(key, glfwGetKey(_window, key) == GLFW_PRESS);
			}
		}
	}

	void Window::processCursorPos(double xpos, double ypos)
	{
		for (CursorPosCallback& callback : _cursorPosCallbacks)
		{
			callback(xpos, ypos);
		}
	}

	void Window::processWindowResize(int width, int height)
	{
		_extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
		for (WindowResizeCallback& callback : _windowResizeCallbacks)
		{
			callback(width, height);
		}
	}

	void Window::operator+=(const KeyCallback& callback)
	{
		_keyCallbacks.emplace_back(callback);
	}

	void Window::operator+=(const CursorPosCallback& callback)
	{
		_cursorPosCallbacks.emplace_back(callback);
	}

	void Window::operator+=(const WindowResizeCallback& callback)
	{
		_windowResizeCallbacks.emplace_back(callback);
	}

	void ProcessCursorPosCallback(GLFWwindow* window, double xpos, double ypos)
	{
		Window* windowPtr = static_cast<Window*>(glfwGetWindowUserPointer(window));
		assert(window != nullptr); // Jihong Shin : Window pointer must be valid
		windowPtr->processCursorPos(xpos, ypos);
	}

	void ProcessWindowResizeCallback(GLFWwindow* window, int width, int height)
	{
		Window* windowPtr = static_cast<Window*>(glfwGetWindowUserPointer(window));
		assert(window != nullptr); // Jihong Shin : Window pointer must be valid
		windowPtr->processWindowResize(width, height);
		windowPtr->setWindowResizedFlag(true);
	}
}