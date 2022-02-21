// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/DebugUtils.h>
#include <VulkanFramework/VulkanExtensions.h>
#include <Common/Logger.h>
#include <string>

namespace vfs
{
	VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtils::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT flags,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* userData)
	{
		switch (severity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			VFS_DEBUG << pCallbackData->pMessage;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			VFS_INFO << pCallbackData->pMessage;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			VFS_WARN << pCallbackData->pMessage;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			VFS_ERROR << pCallbackData->pMessage;
			break;
		default:
			assert(severity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT);
			break;
		}
		return VK_FALSE;
	}

	void DebugUtils::GlfwDebugCallback(int errorCode, const char* description)
	{
		VFS_ERROR << errorCode << ") " << description;
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