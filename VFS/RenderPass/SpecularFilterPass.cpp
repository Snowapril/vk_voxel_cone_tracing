// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <Util/EngineConfig.h>
#include <RenderPass/SpecularFilterPass.h>
#include <RenderPass/RenderPassManager.h>
#include <VulkanFramework/Buffers/Buffer.h>
#include <VulkanFramework/Commands/CommandBuffer.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Images/Image.h>
#include <VulkanFramework/Images/ImageView.h>
#include <VulkanFramework/Images/Sampler.h>
#include <VulkanFramework/Descriptors/DescriptorSet.h>
#include <VulkanFramework/Descriptors/DescriptorSetLayout.h>
#include <VulkanFramework/Descriptors/DescriptorPool.h>
#include <VulkanFramework/Pipelines/GraphicsPipeline.h>
#include <VulkanFramework/Pipelines/PipelineLayout.h>
#include <VulkanFramework/Pipelines/PipelineConfig.h>
#include <VulkanFramework/RenderPass/RenderPass.h>
#include <VulkanFramework/RenderPass/Framebuffer.h>
#include <VulkanFramework/FrameLayout.h>
#include <VulkanFramework/Utils.h>
#include <DirectionalLight.h>
#include <RenderPass/Clipmap/ClipmapRegion.h>
#include <imgui/imgui.h>

namespace vfs
{
	SpecularFilterPass::SpecularFilterPass(CommandPoolPtr cmdPool,
										   VkExtent2D resolution)
	: RenderPassBase(cmdPool), _resolution(resolution)
	{
		// Do nothing
	}

	SpecularFilterPass::~SpecularFilterPass()
	{
		// Do nothing
	}

	void SpecularFilterPass::onBeginRenderPass(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
		std::vector<VkClearValue> clearValues(_attachments.size());
		for (size_t i = 0; i < clearValues.size(); ++i)
		{
			clearValues[i].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		}

		cmdBuffer.beginRenderPass(_renderPass, _framebuffer, clearValues);

		VkViewport viewport = {};
		viewport.x			= 0;
		viewport.y			= 0;
		viewport.minDepth	= 0.0f;
		viewport.maxDepth	= 1.0f;
		viewport.width		= static_cast<float>(_resolution.width);
		viewport.height		= static_cast<float>(_resolution.height);
		cmdBuffer.setViewport({ viewport });

		VkRect2D scissor		= {};
		scissor.offset			= { 0, 0 };
		scissor.extent.width	= _resolution.width;
		scissor.extent.height	= _resolution.height;
		cmdBuffer.setScissor({ scissor });
	}

	void SpecularFilterPass::onEndRenderPass(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
		cmdBuffer.endRenderPass();
	}

	void SpecularFilterPass::onUpdate(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);

		cmdBuffer.bindPipeline(_pipeline);
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 0, { _descriptorSet }, {});

		SpecularFilterPassDesc SpecularFilterPassDesc;
		SpecularFilterPassDesc.tonemapGamma		= _tonemapGamma;
		SpecularFilterPassDesc.tonemapExposure	= _tonemapExposure;
		SpecularFilterPassDesc.tonemapEnable		= static_cast<int32_t>(_tonemapEnable);
		SpecularFilterPassDesc.filterMethod		= _filterMethod;
		cmdBuffer.pushConstants(_pipelineLayout->getLayoutHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SpecularFilterPassDesc), &SpecularFilterPassDesc);
		cmdBuffer.draw(4, 1, 0, 0);
	}

	void SpecularFilterPass::drawGUI(void)
	{
		constexpr const char* kFilterModeLabels[] = {
			 "Gaussian Blur", "Bilateral Filter"
		};

		if (ImGui::TreeNode("Specular Filtering Settings"))
		{
			ImGui::Checkbox("Enable Uncharted Tonemap", &_tonemapEnable);
			ImGui::SliderFloat("Tonemap Gamma", &_tonemapGamma, 0.0f, 3.0f);
			ImGui::SliderFloat("Tonemap Exposure", &_tonemapExposure, 0.0f, 1.0f);
			ImGui::Combo("Specular Filter Method", &_filterMethod, kFilterModeLabels, sizeof(kFilterModeLabels) / sizeof(const char*));
			ImGui::TreePop();
		}
	}

	SpecularFilterPass& SpecularFilterPass::createAttachments(void)
	{
		const VkExtent3D attachmentExtent = { _resolution.width, _resolution.height, 1 };
		
		// 00. Final output hdr attachment
		_attachments.push_back({createAttachment(attachmentExtent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT) });

		// Create sampler for color attachments
		_colorSampler = std::make_shared<Sampler>(_device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_FILTER_LINEAR, 1.0f);

		// Share render target with other render-passes
		_renderPassManager->put("FinalOutputImageView", _attachments[0].imageView.get());
		_renderPassManager->put("FinalOutputSampler",				_colorSampler.get());

#if defined(_DEBUG)
		_debugUtils.setObjectName(_attachments[0].image->getImageHandle(),			"FinalOutputImage"		);
		_debugUtils.setObjectName(_attachments[0].imageView->getImageViewHandle(),	"FinalOutputImageView"	);
		_debugUtils.setObjectName(_colorSampler->getSamplerHandle(),				"FinalOutputSampler"	);
#endif

		return *this;
	}

	SpecularFilterPass& SpecularFilterPass::createRenderPass(void)
	{
		assert(_attachments.empty() == false); // snowapril : Attachments must be filled first
		uint32_t attachmentOffset = 0;

		std::vector<VkAttachmentDescription> attachmentDesc;
		std::vector<VkAttachmentReference>	 colorAttachmentRefs;
		attachmentDesc.reserve(_attachments.size());
		colorAttachmentRefs.reserve(_attachments.size());
		for (size_t i = 0; i < _attachments.size(); ++i)
		{
			VkAttachmentDescription tempDesc = {};
			tempDesc.format			= _attachments[i].image->getImageFormat();
			tempDesc.samples		= VK_SAMPLE_COUNT_1_BIT; // getMaximumSampleCounts(_device);
			tempDesc.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
			tempDesc.storeOp		= VK_ATTACHMENT_STORE_OP_STORE;
			tempDesc.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			tempDesc.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
			tempDesc.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
			tempDesc.finalLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			attachmentDesc.emplace_back(std::move(tempDesc));
			colorAttachmentRefs.push_back({ attachmentOffset++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		}

		std::vector<VkSubpassDependency> subpassDependencies(2, VkSubpassDependency{});
		subpassDependencies[0].srcSubpass		= VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].dstSubpass		= 0;
		subpassDependencies[0].srcStageMask		= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[0].srcAccessMask	= VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[0].dstStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;
		
		subpassDependencies[1].srcSubpass		= 0;
		subpassDependencies[1].dstSubpass		= VK_SUBPASS_EXTERNAL;
		subpassDependencies[1].srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[1].srcAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[1].dstStageMask		= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[1].dstAccessMask	= VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[1].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;
		
		VkSubpassDescription subpassDesc = {};
		subpassDesc.colorAttachmentCount	= static_cast<uint32_t>(colorAttachmentRefs.size());
		subpassDesc.pColorAttachments		= colorAttachmentRefs.data();
		subpassDesc.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;

		_renderPass = std::make_shared<RenderPass>();
		assert(_renderPass->initialize(_device, attachmentDesc, subpassDependencies, { subpassDesc }));
		return *this;
	}

	SpecularFilterPass& SpecularFilterPass::createFramebuffer(void)
	{
		assert(_attachments.empty() == false && _renderPass != nullptr);
		std::vector<VkImageView> imageViews(_attachments.size());
		for (size_t i = 0; i < _attachments.size(); ++i)
		{
			imageViews[i] = _attachments[i].imageView->getImageViewHandle();
		}
		_framebuffer = std::make_shared<Framebuffer>(_device, imageViews, _renderPass->getHandle(), _resolution);
		return *this;
	}

	SpecularFilterPass& SpecularFilterPass::createDescriptors(void)
	{
		// Descriptors for voxel cone tracing
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 },
		};
		_descriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 1, 0);

		_descriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_descriptorLayout->createDescriptorSetLayout(0);
		
		_descriptorSet = std::make_shared<DescriptorSet>(_device, _descriptorPool, _descriptorLayout, 1);
		const ImageView* gbufferContents[] = {
			 _renderPassManager->get<ImageView>("DiffuseContributionImageView"),
			 _renderPassManager->get<ImageView>("SpecularContributionImageView"),
		};
		const Sampler*	 gbufferSampler	= _renderPassManager->get<Sampler>("VCTSampler");
		for (uint32_t i = 0; i < 2; ++i)
		{
			VkDescriptorImageInfo gbufferDescInfo = {};
			gbufferDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			gbufferDescInfo.imageView	= gbufferContents[i]->getImageViewHandle();
			gbufferDescInfo.sampler		= gbufferSampler->getSamplerHandle();
			_descriptorSet->updateImage({ gbufferDescInfo }, i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		}

		return *this;
	}

	SpecularFilterPass& SpecularFilterPass::createPipeline(void)
	{
		VkPushConstantRange renderModePush = {};
		renderModePush.stageFlags	= VK_SHADER_STAGE_FRAGMENT_BIT;
		renderModePush.size			= sizeof(SpecularFilterPassDesc);
		renderModePush.offset		= 0;

		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(
			_device,
			{ _descriptorLayout },
			{ renderModePush  }
		);

		PipelineConfig config;
		PipelineConfig::GetDefaultConfig(&config);
		
		config.inputAseemblyInfo.topology			= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		config.rasterizationInfo.cullMode			= VK_CULL_MODE_NONE;
		config.depthStencilInfo.depthTestEnable		= VK_FALSE;
		config.depthStencilInfo.stencilTestEnable	= VK_FALSE;
		config.renderPass							= _renderPass->getHandle();
		config.pipelineLayout						= _pipelineLayout->getLayoutHandle();

		config.dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		config.dynamicStateInfo.dynamicStateCount	= static_cast<uint32_t>(config.dynamicStates.size());
		config.dynamicStateInfo.pDynamicStates		= config.dynamicStates.data();

		// config.multisampleInfo.rasterizationSamples = getMaximumSampleCounts(_device);
		// if (_device->getDeviceFeature().sampleRateShading == VK_TRUE)
		// {
		// 	config.multisampleInfo.sampleShadingEnable = VK_TRUE;
		// 	config.multisampleInfo.minSampleShading = 0.25f;
		// }
		// else
		// {
		// 	config.multisampleInfo.sampleShadingEnable = VK_FALSE;
		// }

		_pipeline = std::make_shared<GraphicsPipeline>(_device);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_VERTEX_BIT,	"Shaders/specularFilter.vert.spv", nullptr);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/specularFilter.frag.spv", nullptr);
		_pipeline->createPipeline(&config);
		return *this;
	}

	void SpecularFilterPass::processWindowResize(int width, int height) 
	{
		// TODO(snowapril) : another pass that use current attachments must be recreated too
		_resolution = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
		// vkDeviceWaitIdle(_device->getDeviceHandle());

		// createAttachments();
		// createRenderPass();
		// createFramebuffer();
	}
};