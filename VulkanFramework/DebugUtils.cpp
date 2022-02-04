// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/DebugUtils.h>
#include <VulkanFramework/VulkanExtensions.h>
#include <iostream>
#include <string>

namespace vfs
{
	static std::string severityToString(VkDebugUtilsMessageSeverityFlagBitsEXT severity)
	{
		std::string severityString;
		severityString.reserve(8); // As maximum length is WARNING (7 + 1 characters)
		switch (severity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			{
				severityString.assign("VERBOSE");
			}
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			{
				severityString.assign("INFO");
			}
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			{
				severityString.assign("WARNING");
			}
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			{
				severityString.assign("ERROR");
			}
			break;
		default:
			assert(severity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT);
			break;
		}
		return severityString;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtils::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT flags,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* userData)
	{

		std::cerr << "[RenderEngine] " << severityToString(severity) << " -> " << pCallbackData->pMessage << "\n\n";
		return VK_FALSE;
	}

	void DebugUtils::GlfwDebugCallback(int errorCode, const char* description)
	{
		std::cerr << "[RenderEngine] (" << errorCode << ") " << description << '\n';
	}

	void DebugUtils::setObjectName(uint64_t object, const char* name, VkObjectType type)
	{
		// static PFN_vkSetDebugUtilsObjectNameEXT setObjectName = 
		VkDebugUtilsObjectNameInfoEXT nameInfo = {};
		nameInfo.sType			= VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.pNext			= nullptr;
		nameInfo.pObjectName	= name;
		nameInfo.objectHandle	= object;
		nameInfo.objectType		= type;
		vkSetDebugUtilsObjectNameEXT(_device->getDeviceHandle(), &nameInfo);
	}
}