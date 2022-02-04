// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_UTILS_H)
#define VULKAN_FRAMEWORK_UTILS_H

#include <VulkanFramework/pch.h>

namespace vfs
{
	VkSampleCountFlagBits getMaximumSampleCounts(DevicePtr device);
};

#endif