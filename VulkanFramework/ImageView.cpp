// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/ImageView.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Image.h>

namespace vfs
{
	ImageView::ImageView(DevicePtr device, const ImagePtr& targetImage, VkImageAspectFlags aspect, uint32_t mipLevels)
	{
		assert(initialize(device, targetImage, aspect, mipLevels));
	}


	ImageView::ImageView(DevicePtr device, const ImagePtr& targetImage, VkImageViewCreateInfo imageViewInfo)
	{
		assert(initialize(device, targetImage, imageViewInfo));
	}

	ImageView::~ImageView()
	{
		destroyImageView();
	}

	void ImageView::destroyImageView(void)
	{
		vkDestroyImageView(_device->getDeviceHandle(), _imageViewHandle, nullptr);
	}

	// TODO(snowapril) : refactoring two initialize methods
	bool ImageView::initialize(DevicePtr device, const ImagePtr& targetImage, VkImageAspectFlags aspect, uint32_t mipLevels)
	{
		_device = device;

		VkImageViewCreateInfo imageViewInfo				= {};
		imageViewInfo.sType								= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.pNext								= nullptr;
		imageViewInfo.viewType							= static_cast<VkImageViewType>(targetImage->getImageType());
		imageViewInfo.format							= targetImage->getImageFormat();
		imageViewInfo.image								= targetImage->getImageHandle();
		imageViewInfo.components.r						= VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.g						= VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.b						= VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.a						= VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.subresourceRange.aspectMask		= aspect;
		imageViewInfo.subresourceRange.baseMipLevel		= 0;
		imageViewInfo.subresourceRange.levelCount		= mipLevels;
		imageViewInfo.subresourceRange.baseArrayLayer	= 0;
		imageViewInfo.subresourceRange.layerCount		= 1;

		if (vkCreateImageView(_device->getDeviceHandle(), &imageViewInfo, nullptr, &_imageViewHandle) != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}

	// TODO(snowapril) : refactoring two initialize methods
	bool ImageView::initialize(DevicePtr device, const ImagePtr& targetImage,
							   VkImageViewCreateInfo imageViewInfo)
	{
		_device = device;

		imageViewInfo.image = targetImage->getImageHandle();
		if (vkCreateImageView(_device->getDeviceHandle(), &imageViewInfo, nullptr, &_imageViewHandle) != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}
}