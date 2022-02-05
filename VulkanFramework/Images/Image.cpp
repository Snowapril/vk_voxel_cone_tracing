// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Buffers/Buffer.h>
#include <VulkanFramework/Images/Image.h>
#include <VulkanFramework/Commands/CommandPool.h>
#include <VulkanFramework/Commands/CommandBuffer.h>
#include <iostream>

namespace vfs
{
	Image::Image(VmaAllocator allocator, VmaMemoryUsage memoryUsage, const VkImageCreateInfo& imageInfo)
	{
		assert(initialize(allocator, memoryUsage, imageInfo));
	}

	Image::~Image()
	{
		destroyImage();
	}

	void Image::destroyImage(void)
	{
		if (_allocator != nullptr && _imageHandle != VK_NULL_HANDLE && _imageAllocation != nullptr)
		{
			vmaDestroyImage(_allocator, _imageHandle, _imageAllocation);
			_allocator		 = nullptr;
			_imageHandle	 = VK_NULL_HANDLE;
			_imageAllocation = nullptr;
		}
	}

	bool Image::initialize(VmaAllocator allocator, VmaMemoryUsage memoryUsage, const VkImageCreateInfo& imageInfo)
	{
		_allocator	 = allocator;
		_imageFormat = imageInfo.format;
		_dimension	 = imageInfo.extent;
		_imageType	 = imageInfo.imageType;

		VmaAllocationCreateInfo allocationInfo = {};
		allocationInfo.usage		 = memoryUsage;
		allocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vmaCreateImage(_allocator,
			&imageInfo, &allocationInfo,
			&_imageHandle, &_imageAllocation, nullptr) != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}

	VkImageMemoryBarrier Image::generateMemoryBarrier(VkAccessFlags srcMask, VkAccessFlags dstMask, VkImageAspectFlags aspectMask,
													  VkImageLayout srcLayout, VkImageLayout dstLayout) const
	{
		return generateMemoryBarrier(srcMask, dstMask, aspectMask, srcLayout, dstLayout,
									 VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED);
	}

	VkImageMemoryBarrier Image::generateMemoryBarrier(VkAccessFlags srcMask, VkAccessFlags dstMask, VkImageAspectFlags aspectMask,
													  VkImageLayout srcLayout, VkImageLayout dstLayout,
											    	  uint32_t srcQueueFamily, uint32_t dstQueueFamily) const
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType							= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.pNext							= nullptr;
		barrier.image							= _imageHandle;
		barrier.oldLayout						= srcLayout;
		barrier.newLayout						= dstLayout;
		barrier.subresourceRange.aspectMask		= aspectMask;
		barrier.subresourceRange.baseMipLevel	= 0;
		barrier.subresourceRange.levelCount		= 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount		= 1;
		barrier.srcAccessMask					= srcMask;
		barrier.dstAccessMask					= dstMask;
		barrier.srcQueueFamilyIndex				= srcQueueFamily;
		barrier.dstQueueFamilyIndex				= dstQueueFamily;
		return barrier;
	}
}