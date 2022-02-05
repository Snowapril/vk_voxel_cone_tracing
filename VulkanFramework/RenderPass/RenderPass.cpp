// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/RenderPass/RenderPass.h>

namespace vfs
{
	RenderPass::RenderPass(const DevicePtr& device, 
						   const std::vector<VkAttachmentDescription>& attachmentDescs,
						   const std::vector<VkSubpassDependency>& subpassDependencies,
						   const bool isDepthIncluded)
	{
		assert(initialize(device, attachmentDescs, subpassDependencies, isDepthIncluded));
	}

	RenderPass::~RenderPass()
	{
		destroyRenderPass();
	}

	bool RenderPass::initialize(const DevicePtr& device, 
								const std::vector<VkAttachmentDescription>& attachmentDescs,
								const std::vector<VkSubpassDependency>& subpassDependencies,
								const bool isDepthIncluded)
	{
		_device = device;
		const bool isColorIncluded = attachmentDescs.size() > static_cast<size_t>(isDepthIncluded);
		
		// Attachment references
		std::vector<VkAttachmentReference> colorAttachmentRef;
		VkAttachmentReference depthAttachmentRef = {};

		VkSubpassDescription subpassDesc = {};
		subpassDesc.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;

		if (isColorIncluded)
		{
			colorAttachmentRef.reserve(attachmentDescs.size() - static_cast<size_t>(isDepthIncluded));
			for (size_t i = 0; i < attachmentDescs.size() - static_cast<size_t>(isDepthIncluded); ++i)
			{
				colorAttachmentRef.push_back({ static_cast<uint32_t>(i), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			}
			subpassDesc.colorAttachmentCount	= static_cast<uint32_t>(colorAttachmentRef.size());
			subpassDesc.pColorAttachments		= colorAttachmentRef.data();
		}
		else
		{
			subpassDesc.colorAttachmentCount	= 0;
			subpassDesc.pColorAttachments		= nullptr;
		}

		if (isDepthIncluded)
		{
			depthAttachmentRef.attachment	= static_cast<uint32_t>(attachmentDescs.size() - 1);
			depthAttachmentRef.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;
		}
		else
		{
			subpassDesc.pDepthStencilAttachment = nullptr;
		}

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType			= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pNext			= nullptr;
		renderPassInfo.attachmentCount	= static_cast<uint32_t>(attachmentDescs.size());
		renderPassInfo.pAttachments		= attachmentDescs.empty() ? nullptr : attachmentDescs.data();
		renderPassInfo.subpassCount		= 1;
		renderPassInfo.pSubpasses		= &subpassDesc;
		renderPassInfo.dependencyCount	= static_cast<uint32_t>(subpassDependencies.size());
		renderPassInfo.pDependencies	= subpassDependencies.empty() ? nullptr : subpassDependencies.data();

		vkCreateRenderPass(_device->getDeviceHandle(), &renderPassInfo, nullptr, &_renderPassHandle);
		return true;
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