// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_FRAME_LAYOUT_H)
#define VULKAN_FRAMEWORK_FRAME_LAYOUT_H

#include <VulkanFramework/pch.h>

namespace vfs
{
	struct FrameLayout
	{
		DescriptorSetPtr globalDescSet	{ nullptr };
		VkCommandBuffer commandBuffer	{ VK_NULL_HANDLE };
		uint32_t		frameIndex		{	0 };
		float			frameTime		{ 0.0f };
	};
}

#endif