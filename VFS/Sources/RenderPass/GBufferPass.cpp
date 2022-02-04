// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <RenderPass/GBufferPass.h>
#include <VulkanFramework/CommandBuffer.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/DescriptorSetLayout.h>
#include <VulkanFramework/DescriptorSet.h>
#include <VulkanFramework/DescriptorPool.h>
#include <VulkanFramework/Image.h>
#include <VulkanFramework/ImageView.h>
#include <VulkanFramework/Sampler.h>
#include <VulkanFramework/Framebuffer.h>
#include <VulkanFramework/RenderPass.h>
#include <VulkanFramework/GraphicsPipeline.h>
#include <VulkanFramework/PipelineLayout.h>
#include <VulkanFramework/PipelineConfig.h>
#include <VulkanFramework/Camera.h>
#include <VulkanFramework/FrameLayout.h>
#include <RenderPass/RenderPassManager.h>
#include <GLTFScene.h>

namespace vfs
{
	GBufferPass::GBufferPass(DevicePtr device, VkExtent2D resolution)
		: RenderPassBase(device), _gbufferResolution(resolution)
	{
		// Do nothing
	}

	GBufferPass::~GBufferPass()
	{
		_framebuffer.reset();
	}

	void GBufferPass::onBeginRenderPass(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);

		//VkImageMemoryBarrier barrier = _attachments.back().image->generateMemoryBarrier(
		//	VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_MEMORY_WRITE_BIT, VK_IMAGE_ASPECT_DEPTH_BIT,
		//	VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		//);
		//cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 
		//	0, {}, {}, { barrier });

		std::vector<VkClearValue> clearValues(_attachments.size());
		for (size_t i = 0; i < clearValues.size(); ++i)
		{
			if (i == clearValues.size() - 1)
			{
				clearValues[i].depthStencil = { 1.0f, 0 };
			}
			else
			{
				clearValues[i].color = { 0.0f, 0.0f, 0.0f, 0.0f };
			}
		}

		cmdBuffer.beginRenderPass(_renderPass, _framebuffer, clearValues);
		_pipeline->bindPipeline(frameLayout->commandBuffer);

		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		viewport.width = static_cast<float>(_gbufferResolution.width);
		viewport.height = static_cast<float>(_gbufferResolution.height);
		cmdBuffer.setViewport({ viewport });

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent.width = _gbufferResolution.width;
		scissor.extent.height = _gbufferResolution.height;
		
		cmdBuffer.setScissor({ scissor });
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 0,
			{ frameLayout->globalDescSet }, {});
	}

	void GBufferPass::onEndRenderPass(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
		cmdBuffer.endRenderPass();

		//VkImageMemoryBarrier barrier = _attachments.back().image->generateMemoryBarrier(
		//	VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_ASPECT_DEPTH_BIT,
		//	VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		//);
		//cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
		//	0, {}, {}, { barrier });
	}

	void GBufferPass::onUpdate(const FrameLayout* frameLayout)
	{
		(void)frameLayout;
	}

	void GBufferPass::drawUI(void)
	{
		// Print GBuffer Pass elapsed time
	}

	GBufferPass::FramebufferAttachment GBufferPass::createAttachment(VkExtent3D resolution, VkFormat format, VkImageUsageFlags usage)
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
		// imageInfo.samples	= VK_SAMPLE_COUNT_4_BIT;
		ImagePtr image = std::make_shared<Image>(_device->getMemoryAllocator(), VMA_MEMORY_USAGE_GPU_ONLY, imageInfo);
		
		VkImageViewCreateInfo imageViewInfo = ImageView::GetDefaultImageViewInfo();
		imageViewInfo.format	= format;
		imageViewInfo.viewType	= VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.subresourceRange.aspectMask = aspect;
		ImageViewPtr imageView = std::make_shared<ImageView>(_device, image, imageViewInfo);
		
		return { image, imageView };
	}

	GBufferPass& GBufferPass::createAttachments(void)
	{
		const VkExtent3D attachmentExtent = { _gbufferResolution.width, _gbufferResolution.height, 1 };
		
		// 00. Diffuse
		_attachments.push_back({createAttachment(attachmentExtent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)});

		// 01. Normal
		_attachments.push_back({createAttachment(attachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)});

		// 02. Specular
		_attachments.push_back({createAttachment(attachmentExtent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)});

		// 03. Emission
		_attachments.push_back({createAttachment(attachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)});

		// 04. Depth
		_attachments.push_back({createAttachment(attachmentExtent, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)});

		// Create sampler for color attachments
		_colorSampler = std::make_shared<Sampler>(_device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_FILTER_LINEAR, 1.0f);

		// Share gbuffer contents with other render-passes
		_renderPassManager->put("DiffuseImageView",		_attachments[0].imageView.get());
		_renderPassManager->put("NormalImageView",		_attachments[1].imageView.get());
		_renderPassManager->put("SpecularImageView",	_attachments[2].imageView.get());
		_renderPassManager->put("EmissionImageView",	_attachments[3].imageView.get());
		_renderPassManager->put("DepthImageView",		_attachments[4].imageView.get());
		_renderPassManager->put("GBufferSampler",		_colorSampler.get());

#if defined(_DEBUG)
		_debugUtils.setObjectName(_attachments[0].image->getImageHandle(),			"GBuffer(Diffuse)"		);
		_debugUtils.setObjectName(_attachments[0].imageView->getImageViewHandle(),	"GBuffer(Diffuse) View"	);
		_debugUtils.setObjectName(_attachments[1].image->getImageHandle(),			"GBuffer(Normal)"		);
		_debugUtils.setObjectName(_attachments[1].imageView->getImageViewHandle(),	"GBuffer(Normal) View"	);
		_debugUtils.setObjectName(_attachments[2].image->getImageHandle(),			"GBuffer(Specular)"		);
		_debugUtils.setObjectName(_attachments[2].imageView->getImageViewHandle(),	"GBuffer(Specular) View");
		_debugUtils.setObjectName(_attachments[3].image->getImageHandle(),			"GBuffer(Emission)"		);
		_debugUtils.setObjectName(_attachments[3].imageView->getImageViewHandle(),	"GBuffer(Emission) View");
		_debugUtils.setObjectName(_attachments[4].image->getImageHandle(),			"GBuffer(Depth)"		);
		_debugUtils.setObjectName(_attachments[4].imageView->getImageViewHandle(),	"GBuffer(Depth) View"	);
		_debugUtils.setObjectName(_colorSampler->getSamplerHandle(),				"GBuffer Sampler"		);
#endif

		return *this;
	}

	GBufferPass& GBufferPass::createRenderPass(void)
	{
		assert(_attachments.empty() == false); // snowapril : Attachments must be filled first

		std::vector<VkAttachmentDescription> attachmentDesc;
		attachmentDesc.reserve(_attachments.size() - 1); // snowapril : last for depth buffer
		for (size_t i = 0; i < _attachments.size(); ++i)
		{
			VkAttachmentDescription tempDesc = {};
			tempDesc.format			= _attachments[i].image->getImageFormat();
			tempDesc.samples		= VK_SAMPLE_COUNT_1_BIT;
			tempDesc.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
			tempDesc.storeOp		= VK_ATTACHMENT_STORE_OP_STORE;
			tempDesc.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			tempDesc.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
			tempDesc.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
			if (i == _attachments.size() - 1)
			{
				//tempDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				tempDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
			else
			{
				tempDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
			attachmentDesc.emplace_back(std::move(tempDesc));
		}

		std::vector<VkSubpassDependency> subpassDependencies(2, VkSubpassDependency{});
		subpassDependencies[0].srcSubpass		= VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].dstSubpass		= 0;
		subpassDependencies[0].srcStageMask		= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[0].srcAccessMask	= VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[0].dstStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;
		
		subpassDependencies[1].srcSubpass		= VK_SUBPASS_EXTERNAL;
		subpassDependencies[1].dstSubpass		= 0;
		subpassDependencies[1].srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[1].srcAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[1].dstStageMask		= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[1].dstAccessMask	= VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[1].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;

		_renderPass = std::make_shared<RenderPass>(_device, attachmentDesc, subpassDependencies, true);
		return *this;
	}

	GBufferPass& GBufferPass::createFramebuffer(void)
	{
		assert(_attachments.empty() == false && _renderPass != nullptr);
		std::vector<VkImageView> imageViews(_attachments.size());
		for (size_t i = 0; i < _attachments.size(); ++i)
		{
			imageViews[i] = _attachments[i].imageView->getImageViewHandle();
		}
		_framebuffer = std::make_shared<Framebuffer>(_device, imageViews, _renderPass->getHandle(), _gbufferResolution);
		return *this;
	}

	GBufferPass& GBufferPass::createPipeline(const DescriptorSetLayoutPtr& globalDescLayout,
											 const GLTFScene* scene)
	{
		assert(_renderPass != nullptr && _attachments.empty() == false);
		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(_device, { globalDescLayout, scene->getDescriptorLayout() }, { scene->getDefaultPushConstant() });

		PipelineConfig config;
		PipelineConfig::GetDefaultConfig(&config);
		
		const std::vector<VkVertexInputBindingDescription>& bindingDesc = scene->getVertexInputBindingDesc(0);
		const std::vector<VkVertexInputAttributeDescription>& attribDescs = scene->getVertexInputAttribDesc(0);
		config.vertexInputInfo.vertexAttributeDescriptionCount	= static_cast<uint32_t>(attribDescs.size());
		config.vertexInputInfo.pVertexAttributeDescriptions		= attribDescs.data();
		config.vertexInputInfo.vertexBindingDescriptionCount	= static_cast<uint32_t>(bindingDesc.size());;
		config.vertexInputInfo.pVertexBindingDescriptions		= bindingDesc.data();
		
		config.inputAseemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		config.rasterizationInfo.cullMode = VK_CULL_MODE_NONE; // VK_CULL_MODE_BACK_BIT;

		config.dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		config.dynamicStateInfo.dynamicStateCount	= static_cast<uint32_t>(config.dynamicStates.size());
		config.dynamicStateInfo.pDynamicStates		= config.dynamicStates.data();

		config.renderPass	  = _renderPass->getHandle();
		config.pipelineLayout = _pipelineLayout->getLayoutHandle();
		
		config.colorBlendAttachments.clear();
		VkPipelineColorBlendAttachmentState colorBlend = {};
		colorBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
									VK_COLOR_COMPONENT_G_BIT |
									VK_COLOR_COMPONENT_B_BIT |
									VK_COLOR_COMPONENT_A_BIT;
		colorBlend.blendEnable = VK_FALSE;
		config.colorBlendAttachments.resize(_attachments.size() - 1, colorBlend);

		config.colorBlendInfo.attachmentCount = static_cast<uint32_t>(config.colorBlendAttachments.size());
		config.colorBlendInfo.pAttachments = config.colorBlendAttachments.data();
		//config.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT;
		//if (_device->getDeviceFeature().sampleRateShading == VK_TRUE)
		//{
		//	config.multisampleInfo.sampleShadingEnable = VK_TRUE;
		//	config.multisampleInfo.minSampleShading = 0.25f;
		//}
		//else
		//{
		//	config.multisampleInfo.sampleShadingEnable = VK_FALSE;
		//}

		_pipeline = std::make_shared<GraphicsPipeline>(_device);
		_pipeline->addShaderModule(VK_SHADER_STAGE_VERTEX_BIT,	 "Shaders/gBufferPass.vert.spv", nullptr);
		_pipeline->addShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/gBufferPass.frag.spv", nullptr);
		_pipeline->createPipeline(&config);
		return *this;
	}
};