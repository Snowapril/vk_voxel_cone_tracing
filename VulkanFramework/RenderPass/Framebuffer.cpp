// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/RenderPass/Framebuffer.h>
#include <VulkanFramework/Device.h>
#include <iostream>

namespace vfs
{
	Framebuffer::Framebuffer(std::shared_ptr<Device> device,
							 const std::vector<VkImageView>& attachments,
							 VkRenderPass renderPass,
							 VkExtent2D extent)
	{
		assert(initialize(device, attachments, renderPass, extent));
	}

	Framebuffer::~Framebuffer()
	{
		destroyFramebuffer();
	}

	void Framebuffer::destroyFramebuffer(void)
	{
		if (_framebuffer != VK_NULL_HANDLE)
		{
			vkDestroyFramebuffer(_device->getDeviceHandle(), _framebuffer, nullptr);
			_framebuffer = VK_NULL_HANDLE;
		}
		_device.reset();
	}

	bool Framebuffer::initialize(std::shared_ptr<Device> device,
								 const std::vector<VkImageView>& attachments,
								 VkRenderPass renderPass,
								 VkExtent2D extent)
	{
		_device = device;
		_extent = extent;

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType	= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.pNext	= nullptr;
		framebufferInfo.layers	= 1;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments	= attachments.data();
		framebufferInfo.width		= extent.width;
		framebufferInfo.height		= extent.height;
		framebufferInfo.renderPass	= renderPass;

		if (vkCreateFramebuffer(_device->getDeviceHandle(), &framebufferInfo, nullptr, &_framebuffer) != VK_SUCCESS)
		{
			std::cerr << "[RenderEngine] Failed to create framebuffer\n";
			return false;
		}
		return true;
	}
}