// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_DEVICE_H)
#define VULKAN_FRAMEWORK_DEVICE_H

#include <VulkanFramework/pch.h>
#include <memory>

namespace vfs
{
	class Window;

	class Device : NonCopyable
	{
	public:
		explicit Device() = default;
		explicit Device(std::shared_ptr<Window> window);
				~Device();

		struct SwapChainSupportDetails
		{
			VkSurfaceCapabilitiesKHR		capabilities {};
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR>	presentModes;
		};
	public:
		void					destroyDevice				();
		bool					initialize					(std::shared_ptr<Window> window);
		bool					initializeLogicalDevice		(const std::vector<uint32_t>& queueFamilyIndices);
		bool					initializeMemoryAllocator	(void);
		SwapChainSupportDetails querySwapChainSupport		();
		void					findQueueFamilyIndices		(OUT uint32_t* graphicsFamily, 
															 OUT uint32_t* presentFamily,
															 OUT uint32_t* loaderFamily);

		inline VmaAllocator		getMemoryAllocator(void) const
		{
			return _memoryAllocator;
		}
		inline VkSurfaceKHR		getSurfaceHandle(void) const
		{
			return _surface;
		}
		inline VkInstance		getVulkanInstance(void) const
		{
			return _instance;
		}
		inline VkPhysicalDevice getPhysicalDeviceHandle(void) const
		{
			return _physicalDevice;
		}
		inline VkDevice			getDeviceHandle(void) const
		{
			return _device;
		}
		inline const VkPhysicalDeviceProperties& getDeviceProperty(void) const
		{
			return _physicalDeviceProperties;
		}
		inline const VkPhysicalDeviceFeatures& getDeviceFeature(void) const
		{
			return _physicalDeviceFeatures;
		}

	private:	
		bool initializeInstance			(void);
		bool pickPhysicalDevice			(void);
		bool checkExtensionSupport		(void) const;
		bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;
		bool checkDeviceSuitable		(VkPhysicalDevice device) const;
		void queryDebugUtilsCreateInfo	(OUT VkDebugUtilsMessengerCreateInfoEXT* desc) const;
		std::vector<const char*> getRequiredExtensions(void) const;

	private:
		VkPhysicalDeviceProperties	_physicalDeviceProperties	{		0,	 	 };
		VkPhysicalDeviceFeatures	_physicalDeviceFeatures		{		0,		 };
		VkInstance					_instance					{ VK_NULL_HANDLE };
		VkPhysicalDevice			_physicalDevice				{ VK_NULL_HANDLE };
		VkDevice					_device						{ VK_NULL_HANDLE };
		VkDebugUtilsMessengerEXT	_debugMessenger				{ VK_NULL_HANDLE };
		VkSurfaceKHR				_surface					{ VK_NULL_HANDLE };
		VmaAllocator				_memoryAllocator			{	nullptr		 };
		std::shared_ptr<Window>		_window						{	nullptr		 };
		bool						_enableValidationLayer		{	false		 };
	};
}

#endif