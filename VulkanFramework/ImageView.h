// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_IMAGE_VIEW_H)
#define VULKAN_FRAMEWORK_IMAGE_VIEW_H

#include <VulkanFramework/pch.h>

namespace vfs
{
	class ImageView : NonCopyable
	{
	public:
		explicit ImageView() = default;
		explicit ImageView(DevicePtr device, const ImagePtr& targetImage, 
						   VkImageAspectFlags aspect, uint32_t mipLevels);
		explicit ImageView(DevicePtr device, const ImagePtr& targetImage, 
						   VkImageViewCreateInfo imageViewInfo);
				~ImageView();

	public:
		void destroyImageView	(void);
		bool initialize			(DevicePtr device, const ImagePtr& targetImage, 
								 VkImageAspectFlags aspect, uint32_t mipLevels);
		bool initialize			(DevicePtr device, const ImagePtr& targetImage, 
								 VkImageViewCreateInfo imageViewInfo);

		static VkImageViewCreateInfo GetDefaultImageViewInfo(void)
		{
			VkImageViewCreateInfo depthViewInfo = {};
			depthViewInfo.sType			= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			depthViewInfo.pNext			= nullptr;
			depthViewInfo.components.r	= VK_COMPONENT_SWIZZLE_IDENTITY;
			depthViewInfo.components.g	= VK_COMPONENT_SWIZZLE_IDENTITY;
			depthViewInfo.components.b	= VK_COMPONENT_SWIZZLE_IDENTITY;
			depthViewInfo.components.a	= VK_COMPONENT_SWIZZLE_IDENTITY;
			depthViewInfo.subresourceRange.baseMipLevel		= 0;
			depthViewInfo.subresourceRange.levelCount		= 1;
			depthViewInfo.subresourceRange.baseArrayLayer	= 0;
			depthViewInfo.subresourceRange.layerCount		= 1;
			return depthViewInfo;
		}

		inline VkImageView getImageViewHandle(void) const
		{
			return _imageViewHandle;
		}

	private:
		DevicePtr	_device				{ nullptr };
		VkImageView	_imageViewHandle	{ VK_NULL_HANDLE };
	};
}

#endif