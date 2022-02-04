// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/VulkanExtensions.h>


#ifdef VK_EXT_debug_utils
static PFN_vkSetDebugUtilsObjectNameEXT		pfn_vkSetDebugUtilsObjectNameEXT	= 0;
static PFN_vkCmdBeginDebugUtilsLabelEXT		pfn_vkCmdBeginDebugUtilsLabelEXT	= 0;
static PFN_vkCmdEndDebugUtilsLabelEXT		pfn_vkCmdEndDebugUtilsLabelEXT		= 0;
static PFN_vkCmdInsertDebugUtilsLabelEXT	pfn_vkCmdInsertDebugUtilsLabelEXT	= 0;
static PFN_vkCreateDebugUtilsMessengerEXT	pfn_vkCreateDebugUtilsMessengerEXT	= 0;
static PFN_vkDestroyDebugUtilsMessengerEXT	pfn_vkDestroyDebugUtilsMessengerEXT = 0;

VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectNameEXT(
	VkDevice device,
	const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
{
	return pfn_vkSetDebugUtilsObjectNameEXT(device, pNameInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo)
{
	return pfn_vkCmdBeginDebugUtilsLabelEXT(commandBuffer, pLabelInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer)
{
	return pfn_vkCmdEndDebugUtilsLabelEXT(commandBuffer);
}

VKAPI_ATTR void VKAPI_CALL vkCmdInsertDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo)
{
	return pfn_vkCmdInsertDebugUtilsLabelEXT(commandBuffer, pLabelInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pMessenger)
{
	return pfn_vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT messenger,
	const VkAllocationCallbacks* pAllocator)
{
	return pfn_vkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}
#endif

namespace vfs
{
	void initializeVulkanExtensions(VkInstance instance)
	{
#ifdef VK_EXT_debug_utils
#pragma warning (push)
#pragma warning (disable : 4191)
		pfn_vkSetDebugUtilsObjectNameEXT	= (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
		pfn_vkCreateDebugUtilsMessengerEXT	= (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		pfn_vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		pfn_vkCmdBeginDebugUtilsLabelEXT	= (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT");
		pfn_vkCmdEndDebugUtilsLabelEXT		= (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT");
		pfn_vkCmdInsertDebugUtilsLabelEXT	= (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT");
#pragma warning (pop)
#endif
	}
}