// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/RenderPass/RenderPass.h>

namespace vfs
{
	RenderPass::RenderPass(const DevicePtr& device, 
						   const std::vector<VkAttachmentDescription>& attachmentDescs,
						   const std::vector<VkSubpassDependency>& subpassDependencies,
						   const std::vector<VkSubpassDescription>& subpassDescs)
	{
		assert(initialize(device, attachmentDescs, subpassDependencies, subpassDescs));
	}

	RenderPass::~RenderPass()
	{
		destroyRenderPass();
	}

	bool RenderPass::initialize(const DevicePtr& device, 
								const std::vector<VkAttachmentDescription>& attachmentDescs,
								const std::vector<VkSubpassDependency>& subpassDependencies,
								const std::vector<VkSubpassDescription>& subpassDescs)
	{
		_device = device;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType			= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pNext			= nullptr;
		renderPassInfo.attachmentCount	= static_cast<uint32_t>(attachmentDescs.size());
		renderPassInfo.pAttachments		= attachmentDescs.empty() ? nullptr : attachmentDescs.data();
		renderPassInfo.subpassCount		= static_cast<uint32_t>(subpassDescs.size());
		renderPassInfo.pSubpasses		= subpassDescs.empty() ? nullptr : subpassDescs.data();
		renderPassInfo.dependencyCount	= static_cast<uint32_t>(subpassDependencies.size());
		renderPassInfo.pDependencies	= subpassDependencies.empty() ? nullptr : subpassDependencies.data();

		return vkCreateRenderPass(_device->getDeviceHandle(), &renderPassInfo, nullptr, &_renderPassHandle)
			== VK_SUCCESS;
	}

	void RenderPass::destroyRenderPass(void)
	{
		if (_renderPassHandle != VK_NULL_HANDLE)
		{
			vkDestroyRenderPass(_device->getDeviceHandle(), _renderPassHandle, nullptr);
			_renderPassHandle = VK_NULL_HANDLE;
		}
		_device.reset();
	}
}