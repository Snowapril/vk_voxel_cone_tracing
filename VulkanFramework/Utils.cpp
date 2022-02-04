// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Device.h>

namespace vfs
{
	VkSampleCountFlagBits getMaximumSampleCounts(DevicePtr device)
	{
		const VkPhysicalDeviceProperties& deviceProperty = device->getDeviceProperty();
		const VkSampleCountFlags sampleCountLimit = deviceProperty.limits.framebufferDepthSampleCounts;
		if (sampleCountLimit & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
		if (sampleCountLimit & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
		if (sampleCountLimit & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
		if (sampleCountLimit &  VK_SAMPLE_COUNT_8_BIT) return  VK_SAMPLE_COUNT_8_BIT;
		if (sampleCountLimit &  VK_SAMPLE_COUNT_4_BIT) return  VK_SAMPLE_COUNT_4_BIT;
		if (sampleCountLimit &  VK_SAMPLE_COUNT_2_BIT) return  VK_SAMPLE_COUNT_2_BIT;
		return VK_SAMPLE_COUNT_1_BIT;
	}
};