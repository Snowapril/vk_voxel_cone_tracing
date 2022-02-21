// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <RenderPass/RenderPassBase.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/Commands/CommandPool.h>
#include <VulkanFramework/Images/Image.h>
#include <VulkanFramework/Images/ImageView.h>

namespace vfs
{
	RenderPassBase::RenderPassBase(CommandPoolPtr cmdPool)
		: _cmdPool(cmdPool),
		  _queue(_cmdPool->getQueue()),
		  _device(_queue->getDevicePtr()), 
		  _debugUtils(_device)
	{
		// Do nothing
	}

	RenderPassBase::RenderPassBase(CommandPoolPtr cmdPool,
								   RenderPassPtr renderPass)
		: _cmdPool(cmdPool),
		  _queue(_cmdPool->getQueue()),
		  _device(_queue->getDevicePtr()), 
		  _renderPass(renderPass),
		  _debugUtils(_device)
	{
		// Do nothing
	}

	RenderPassBase::~RenderPassBase()
	{
		_attachments.clear();
		_pipelineLayout.reset();
		_pipeline.reset();
		_renderPass.reset();
		_device.reset();
		_queue.reset();
		_cmdPool.reset();
		_renderPassManager = nullptr;
	}

	void RenderPassBase::beginRenderPass(const FrameLayout* frameLayout)
	{
		onBeginRenderPass(frameLayout);
	}

	void RenderPassBase::endRenderPass(const FrameLayout* frameLayout)
	{
		onEndRenderPass(frameLayout);
	}

	void RenderPassBase::update(const FrameLayout* frameLayout)
	{
		onUpdate(frameLayout);
	}

	void RenderPassBase::render(const FrameLayout* frameLayout)
	{
		// TODO(snowapril) : use own command buffer and queue submit, sync here
		onBeginRenderPass(frameLayout);
		onUpdate(frameLayout);
		onEndRenderPass(frameLayout);
	}

	void RenderPassBase::attachRenderPassManager(RenderPassManager* renderPassManager)
	{
		_renderPassManager = renderPassManager;
	}

	RenderPassBase::FramebufferAttachment RenderPassBase::createAttachment(VkExtent3D resolution, VkFormat format, 
																		   VkImageUsageFlags usage, VkSampleCountFlagBits sampleCount)
	{
		usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		
		VkImageAspectFlags aspect = 0;
		if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		
		VkImageCreateInfo imageInfo = Image::GetDefaultImageCreateInfo();
		imageInfo.extent	= resolution;
		imageInfo.format	= format;
		imageInfo.usage		= usage;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.samples	= sampleCount;
		ImagePtr image = std::make_shared<Image>(_device->getMemoryAllocator(), VMA_MEMORY_USAGE_GPU_ONLY, imageInfo);
		
		VkImageViewCreateInfo imageViewInfo = ImageView::GetDefaultImageViewInfo();
		imageViewInfo.format	= format;
		imageViewInfo.viewType	= VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.subresourceRange.aspectMask = aspect;
		ImageViewPtr imageView = std::make_shared<ImageView>(_device, image, imageViewInfo);
		
		return { image, imageView };
	}
};