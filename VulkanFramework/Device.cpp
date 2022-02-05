// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/DebugUtils.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/VulkanExtensions.h>
#include <VulkanFramework/Window.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <set>
#include <iostream>

constexpr const char* REQUIRED_EXTENSIONS[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
constexpr const char* REQUIRED_LAYERS[]		= { "VK_LAYER_KHRONOS_validation"	};

namespace vfs
{
	Device::Device(std::shared_ptr<Window> window)
	{
		// Initialization call here
		assert(initialize(window));
	}

	Device::~Device()
	{
		destroyDevice();
	}

	void Device::destroyDevice()
	{
		if (_memoryAllocator != nullptr)
		{
			vmaDestroyAllocator(_memoryAllocator);
			_memoryAllocator = nullptr;
		}

		if (_device != VK_NULL_HANDLE)
		{
			vkDestroyDevice(_device, nullptr);
			_device = VK_NULL_HANDLE;
		}

		if (_enableValidationLayer && _debugMessenger != VK_NULL_HANDLE)
		{
			vkDestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
			_debugMessenger = VK_NULL_HANDLE;
		}

		if (_instance != VK_NULL_HANDLE)
		{
			if (_surface != VK_NULL_HANDLE)
			{
				vkDestroySurfaceKHR(_instance, _surface, nullptr);
				_instance = VK_NULL_HANDLE;
			}
			vkDestroyInstance(_instance, nullptr);
			_instance = VK_NULL_HANDLE;
		}
		_window.reset();
	}

	bool Device::initialize(std::shared_ptr<Window> window)
	{
#ifdef NDEBUG
		_enableValidationLayer = false;
#else
		_enableValidationLayer = true;
#endif
		_window = window;

		if (_enableValidationLayer)
		{
			uint32_t layerCount;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
			std::vector<VkLayerProperties> layerProperties(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());

			bool layerFound{ false };
			for (const VkLayerProperties& property : layerProperties)
			{
				if (strcmp(property.layerName, "VK_LAYER_KHRONOS_validation") == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				std::cerr << "[RenderEngine] Failed to find validation layer in this device\n";
				_enableValidationLayer = false;
			}
			else
			{
				std::clog << "[RenderEngine] Validation layer found\n";
			}
		}

		if (!initializeInstance())
		{
			return false;
		}

		initializeVulkanExtensions(_instance);

		if (_enableValidationLayer)
		{
			VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
			queryDebugUtilsCreateInfo(&debugInfo);

			if (vkCreateDebugUtilsMessengerEXT(_instance, &debugInfo, nullptr, &_debugMessenger) != VK_SUCCESS)
			{
				std::cerr << "[RenderEngine] Failed to create debug messenger\n";
				return false;
			}
			std::clog << "[RenderEngine] Debug messenger created\n";
		}

		if (!_window->getWindowSurface(_instance, &_surface))
		{
			return false;
		}

		if (!pickPhysicalDevice())
		{
			return false;
		}

		return true;
	}

	bool Device::initializeInstance()
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext				= nullptr;
		appInfo.pApplicationName	= _window->getTitle();
		appInfo.applicationVersion	= VK_MAKE_VERSION(0, 0, 0);
		appInfo.pEngineName			= "PAEngine";
		appInfo.apiVersion			= VK_API_VERSION_1_2;

		VkInstanceCreateInfo instanceInfo = {};
		instanceInfo.sType				= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.pNext				= nullptr;
		instanceInfo.pApplicationInfo	= &appInfo;

		std::vector<const char*> extensions = getRequiredExtensions();
		instanceInfo.enabledExtensionCount	 = static_cast<uint32_t>(extensions.size());
		instanceInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
		if (_enableValidationLayer)
		{
			instanceInfo.enabledLayerCount	 = sizeof(REQUIRED_LAYERS) / sizeof(const char*);
			instanceInfo.ppEnabledLayerNames = REQUIRED_LAYERS;

			queryDebugUtilsCreateInfo(&debugInfo);
			instanceInfo.pNext = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugInfo);
		}
		else
		{
			instanceInfo.enabledLayerCount	 = 0;
			instanceInfo.ppEnabledLayerNames = nullptr;
		}

		if (vkCreateInstance(&instanceInfo, nullptr, &_instance) != VK_SUCCESS)
		{
			return false;
		}

		if (!checkExtensionSupport())
		{
			return false;
		}

		return true;
	}

	void Device::queryDebugUtilsCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* desc) const
	{
		desc->sType				= VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		desc->pNext				= nullptr;
		desc->pUserData			= nullptr;
		desc->messageSeverity	= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT	|
								  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		desc->messageType		= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT		|
								  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT	|
								  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		desc->pfnUserCallback	= DebugUtils::DebugCallback;
	}

	bool Device::pickPhysicalDevice()
	{
		uint32_t physicalDeviceCount{};
		vkEnumeratePhysicalDevices(_instance, &physicalDeviceCount, nullptr);
		std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
		vkEnumeratePhysicalDevices(_instance, &physicalDeviceCount, physicalDevices.data());

		for (auto device : physicalDevices)
		{
			if (checkDeviceSuitable(device))
			{
				_physicalDevice = device;
				break;
			}
		}

		if (_physicalDevice == VK_NULL_HANDLE)
		{
			std::cerr << "[RenderEngine] No Available vulkan device\n";
			return false;
		}

		vkGetPhysicalDeviceProperties(_physicalDevice, &_physicalDeviceProperties);
		vkGetPhysicalDeviceFeatures(_physicalDevice, &_physicalDeviceFeatures);
		std::clog << "[RenderEngine] Selected Physical Device : " << _physicalDeviceProperties.deviceName << '\n';
		return true;
	}

	bool Device::initializeLogicalDevice(const std::vector<uint32_t>& queueFamilyIndices)
	{
		constexpr float QUEUE_PRIORITY = 1.0f;
		std::set<uint32_t> uniqueQueueFamilies(queueFamilyIndices.begin(), queueFamilyIndices.end());
		
		std::vector<VkDeviceQueueCreateInfo> queueInfos;
		queueInfos.reserve(uniqueQueueFamilies.size());
		for (const uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueInfo = {};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.pNext = nullptr;
			queueInfo.queueCount = 1;
			queueInfo.queueFamilyIndex = queueFamily;
			queueInfo.pQueuePriorities = &QUEUE_PRIORITY;
			queueInfos.emplace_back(std::move(queueInfo));
		}

		// TODO(snowapril) : support for device feature control
		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy					  = VK_TRUE;
		deviceFeatures.fragmentStoresAndAtomics				  = VK_TRUE;
		deviceFeatures.geometryShader						  = VK_TRUE;
		deviceFeatures.multiDrawIndirect					  = VK_TRUE;
		deviceFeatures.fillModeNonSolid						  = VK_TRUE;
		deviceFeatures.sampleRateShading					  = VK_TRUE;
		deviceFeatures.multiViewport						  = VK_TRUE;
		deviceFeatures.vertexPipelineStoresAndAtomics		  = VK_TRUE;
		deviceFeatures.shaderTessellationAndGeometryPointSize = VK_TRUE;

		// TODO(snowapril) : support for device feature control
		VkPhysicalDeviceDescriptorIndexingFeatures descIndexingFeatures = {};
		descIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		descIndexingFeatures.descriptorBindingPartiallyBound				= VK_TRUE;
		descIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind	= VK_TRUE;
		descIndexingFeatures.descriptorBindingUpdateUnusedWhilePending		= VK_TRUE;

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType					= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pNext					= &descIndexingFeatures;
		deviceCreateInfo.queueCreateInfoCount	= static_cast<uint32_t>(queueInfos.size());
		deviceCreateInfo.pQueueCreateInfos		= queueInfos.data();
		deviceCreateInfo.pEnabledFeatures		= &deviceFeatures;
		// TODO(snowapril) : Add control over extension and validation layer
		
		deviceCreateInfo.enabledExtensionCount	 = sizeof(REQUIRED_EXTENSIONS) / sizeof(const char*);
		deviceCreateInfo.ppEnabledExtensionNames = REQUIRED_EXTENSIONS;

		if (_enableValidationLayer)
		{
			deviceCreateInfo.enabledLayerCount	 = sizeof(REQUIRED_LAYERS) / sizeof(const char*);
			deviceCreateInfo.ppEnabledLayerNames = REQUIRED_LAYERS;
		}
		else
		{
			deviceCreateInfo.enabledLayerCount	 = 0;
			deviceCreateInfo.ppEnabledLayerNames = nullptr;
		}

		if (vkCreateDevice(_physicalDevice, &deviceCreateInfo, nullptr, &_device) != VK_FALSE)
		{
			return false;
		}
		std::clog << "[RenderEngine] Logical device created\n";

		return true;
	}

	bool Device::initializeMemoryAllocator(void)
	{
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = _physicalDevice;
		allocatorInfo.device		 = _device;
		allocatorInfo.instance		 = _instance;
		// TODO(snowapril) : may need to add more config for allocator

		if (vmaCreateAllocator(&allocatorInfo, &_memoryAllocator) != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}

	bool Device::checkDeviceSuitable(VkPhysicalDevice device) const
	{
		// TODO(snowapril) : findQueueFamilyIndices(device) check completeness
		// TODO(snowapril) : checkDeviceExtensionSupport(device) check true
		// TODO(snowapril) : add check querySwapChainSupport formats, presentModes not empty

		return true;
	}

	bool Device::checkDeviceExtensionSupport(VkPhysicalDevice device) const
	{
		// TODO(snowapril) : Add device suitability check here
		uint32_t deviceExtensionPropertyCount{ 0 };
		vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionPropertyCount, nullptr);
		std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionPropertyCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionPropertyCount, deviceExtensions.data());

		// TODO(snowapril) : check REQUIRED_EXTENSIONS are all in the deviceExtensions
		return true;
	}

	void Device::findQueueFamilyIndices(uint32_t* graphicsFamily,
										uint32_t* presentFamily,
										uint32_t* loaderFamily)
	{
		uint32_t queueFamilyCount{ 0 };
		vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

		// Find queue family which support both graphics & present
		uint32_t i = 0;
		bool graphicsBits { false }, presentBits { false }, loaderBits { false };
		for (const VkQueueFamilyProperties& queueFamilyProperty : queueFamilyProperties)
		{
			if (queueFamilyProperty.queueCount > 0 && 
				(queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT))
			{
				*graphicsFamily = i;
				graphicsBits = true;
			}

			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice, i, _surface, &presentSupport);
			if (queueFamilyProperty.queueCount > 0 && presentSupport == VK_TRUE)
			{
				*presentFamily = i;
				presentBits = true;
			}

			if (graphicsBits && presentBits)
			{
				break;
			}
			else
			{
				++i;
			}
		}

		i = 0;
		for (const VkQueueFamilyProperties& queueFamilyProperty : queueFamilyProperties)
		{
			if (queueFamilyProperty.queueCount > 0 && 
				(queueFamilyProperty.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
				(queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
				(queueFamilyProperty.queueFlags & VK_QUEUE_COMPUTE_BIT))
			{
				*loaderFamily = i;
				loaderBits = true;
			}

			if (loaderBits && *loaderFamily != *graphicsFamily) // snowapril : prefer separate family for loader queue
			{
				break;
			}
			else
			{
				++i;
			}
		}
	}

	bool Device::checkExtensionSupport() const
	{
		uint32_t extensionCount {};
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		
		std::vector<VkExtensionProperties> properties(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, properties.data());

		std::clog << "[RenderEngine] Enabled extensions :\n";
		for (const auto& extensionProperty : properties)
		{
			std::clog << '\t' << extensionProperty.extensionName << '\n';
		}

		std::vector<const char*> requiredExtensions = getRequiredExtensions();
		// TODO(snowapril) : compare properties and required extensions here.

		return true;
	}

	Device::SwapChainSupportDetails Device::querySwapChainSupport()
	{
		Device::SwapChainSupportDetails detail;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &detail.capabilities);

		uint32_t surfaceFormatCount{ 0 };
		vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &surfaceFormatCount, nullptr);

		if (surfaceFormatCount != 0)
		{
			detail.formats.resize(surfaceFormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &surfaceFormatCount, detail.formats.data());
		}

		uint32_t presentModeCount{ 0 };
		vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			detail.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, detail.presentModes.data());
		}

		return detail;
	}

	std::vector<const char*> Device::getRequiredExtensions() const
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if (_enableValidationLayer)
		{
			extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}
}