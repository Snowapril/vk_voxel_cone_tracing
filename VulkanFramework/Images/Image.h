// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_IMAGE_H)
#define VULKAN_FRAMEWORK_IMAGE_H

#include <VulkanFramework/pch.h>

namespace vfs
{
	class Image : NonCopyable
	{
	public:
		explicit Image() = default;
		explicit Image(VmaAllocator allocator, VmaMemoryUsage memoryUsage, const VkImageCreateInfo& imageInfo);
				~Image();

	public:
		void destroyImage	(void);
		bool initialize		(VmaAllocator allocator, VmaMemoryUsage memoryUsage, const VkImageCreateInfo& imageInfo);

		VkImageMemoryBarrier generateMemoryBarrier(VkAccessFlags srcMask, VkAccessFlags dstMask, VkImageAspectFlags aspectMask,
												   VkImageLayout srcLayout, VkImageLayout dstLayout) const;
		VkImageMemoryBarrier generateMemoryBarrier(VkAccessFlags srcMask, VkAccessFlags dstMask, VkImageAspectFlags aspectMask,
												   VkImageLayout srcLayout, VkImageLayout dstLayout,
												   uint32_t srcQueueFamily, uint32_t dstQueueFamily) const;

		static inline VkImageCreateInfo GetDefaultImageCreateInfo(void)
		{
			VkImageCreateInfo imageInfo = {};
			imageInfo.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.pNext			= nullptr;
			imageInfo.mipLevels		= 1;
			imageInfo.arrayLayers	= 1;
			imageInfo.samples		= VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.tiling		= VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			return imageInfo;
		}

		inline VkImage getImageHandle(void) const
		{
			return _imageHandle;
		}
		inline VkExtent3D getDimension(void) const
		{
			return _dimension;
		}
		inline VkFormat getImageFormat(void) const
		{
			return _imageFormat;
		}
		inline VkImageType getImageType(void) const
		{
			return _imageType;
		}

	private:
		VmaAllocator			_allocator			{ nullptr };
		VkImage					_imageHandle		{ VK_NULL_HANDLE };
		VmaAllocation			_imageAllocation	{ nullptr };
		VkExtent3D				_dimension			{ 0, 0 ,0 };
		VkFormat				_imageFormat		{ VK_FORMAT_UNDEFINED };
		VkImageType				_imageType			{ VK_IMAGE_TYPE_MAX_ENUM };
	};
}

#endif