// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_WINDOW_H)
#define VULKAN_FRAMEWORK_WINDOW_H

#include <VulkanFramework/pch.h>
#include <functional>

// Forward declarations
struct GLFWwindow;

namespace vfs 
{
	class Window : NonCopyable
	{
	public:
		using KeyCallback			= std::function<void(uint32_t, bool)>;
		using CursorPosCallback		= std::function<void(double xpos, double ypos)>;
		using WindowResizeCallback	= std::function<void(int, int)>;

		explicit Window(const char* title, int width, int height);
				~Window();

	public:
		void		 destroyWindow		 (void);
		bool		 initialize			 (const char* title, int width, int height);
		void		 processKeyInput	 (void);
		void		 processCursorPos	 (double xpos, double ypos);
		void		 processWindowResize (int width, int height);
		VkSurfaceKHR getWindowSurface	 (VkInstance instance);

		void operator+=(const KeyCallback&			callback);
		void operator+=(const CursorPosCallback&	callback);
		void operator+=(const WindowResizeCallback& callback);

		inline GLFWwindow*	getWindowHandle(void) const
		{
			return _window;
		}
		inline const char*  getTitle(void) const
		{
			return _title;
		}
		inline bool			getWindowShouldClose(void) const
		{
			return glfwWindowShouldClose(_window) == GLFW_TRUE;
		}
		inline float		getAspectRatio(void) const
		{
			return static_cast<float>(_extent.x) / static_cast<float>(_extent.y);
		}
		inline VkExtent2D	getWindowExtent(void) const
		{
			return { static_cast<uint32_t>(_extent.x), static_cast<uint32_t>(_extent.y) };
		}
	private:
		std::vector<KeyCallback>			_keyCallbacks;
		std::vector<CursorPosCallback>		_cursorPosCallbacks;
		std::vector<WindowResizeCallback>	_windowResizeCallbacks;
		GLFWwindow* _window { nullptr };
		const char* _title  { nullptr };
		glm::ivec2	_extent	{   0, 0  };
	};
}

#endif