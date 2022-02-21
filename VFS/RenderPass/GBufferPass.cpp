// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <RenderPass/GBufferPass.h>
#include <RenderPass/RenderPassManager.h>
#include <VulkanFramework/Commands/CommandBuffer.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Descriptors/DescriptorSetLayout.h>
#include <VulkanFramework/Descriptors/DescriptorSet.h>
#include <VulkanFramework/Descriptors/DescriptorPool.h>
#include <VulkanFramework/Images/Image.h>
#include <VulkanFramework/Images/ImageView.h>
#include <VulkanFramework/Images/Sampler.h>
#include <VulkanFramework/RenderPass/Framebuffer.h>
#include <VulkanFramework/RenderPass/RenderPass.h>
#include <VulkanFramework/Pipelines/GraphicsPipeline.h>
#include <VulkanFramework/Pipelines/PipelineLayout.h>
#include <VulkanFramework/Pipelines/PipelineConfig.h>
#include <VulkanFramework/FrameLayout.h>
#include <VulkanFramework/Utils.h>
#include <Camera.h>
#include <SceneManager.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>

namespace vfs
{
	GBufferPass::GBufferPass(CommandPoolPtr cmdPool, VkExtent2D resolution)
		: RenderPassBase(cmdPool), _gbufferResolution(resolution)
	{
		// Do nothing
	}

	GBufferPass::GBufferPass(CommandPoolPtr cmdPool, VkExtent2D resolution,
							 const DescriptorSetLayoutPtr& globalDescLayout)
		: RenderPassBase(cmdPool)
	{
		assert(initializeGBufferPass(resolution, globalDescLayout));
		assert(initializeDebugPass());
	}

	GBufferPass::~GBufferPass()
	{
		_framebuffer.reset();
	}

	bool GBufferPass::initializeGBufferPass(VkExtent2D resolution,
											const DescriptorSetLayoutPtr& globalDescLayout)
	{
		_gbufferResolution = resolution;

		createAttachments();
		createRenderPass();
		createFramebuffer();
		createPipeline(globalDescLayout);
		return true;
	}

	bool GBufferPass::initializeDebugPass(void)
	{
		VkSampler commonSampler = _colorSampler->getSamplerHandle();

		_gbufferDebugDescSets.push_back(ImGui_ImplVulkan_AddTexture(
			commonSampler, _attachments[0].imageView->getImageViewHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		));
		_gbufferDebugDescSets.push_back(ImGui_ImplVulkan_AddTexture(
			commonSampler, _attachments[1].imageView->getImageViewHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		));
		_gbufferDebugDescSets.push_back(ImGui_ImplVulkan_AddTexture(
			commonSampler, _attachments[2].imageView->getImageViewHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		));
		_gbufferDebugDescSets.push_back(ImGui_ImplVulkan_AddTexture(
			commonSampler, _attachments[3].imageView->getImageViewHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		));
		_gbufferDebugDescSets.push_back(ImGui_ImplVulkan_AddTexture(
			commonSampler, _attachments[4].imageView->getImageViewHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		));
		return true;
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
				clearValues[i].color = { 0.0f, 0.0f, 0.0f, 1.0f };
			}
		}

		cmdBuffer.beginRenderPass(_renderPass, _framebuffer, clearValues);
		_pipeline->bindPipeline(frameLayout->commandBuffer);

		VkViewport viewport = {};
		viewport.x			= 0;
		viewport.y			= 0;
		viewport.minDepth	= 0.0f;
		viewport.maxDepth	= 1.0f;
		viewport.width		= static_cast<float>(_gbufferResolution.width);
		viewport.height		= static_cast<float>(_gbufferResolution.height);
		cmdBuffer.setViewport({ viewport });

		VkRect2D scissor = {};
		scissor.offset			= { 0, 0 };
		scissor.extent.width	= _gbufferResolution.width;
		scissor.extent.height	= _gbufferResolution.height;
		
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
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
		
		SceneManager* sceneManager = _renderPassManager->get<SceneManager>("SceneManager");
		sceneManager->cmdDraw(frameLayout->commandBuffer, _pipelineLayout, 0);
	}

	void GBufferPass::drawGUI(void)
	{
		// Print GBuffer Pass elapsed time
	}

	void GBufferPass::drawDebugInfo(void)
	{
		if (ImGui::TreeNode("GBuffer Images"))
		{
			ImGui::Image(_gbufferDebugDescSets[0], ImVec2(64.0f, 64.0f));
			ImGui::SameLine();
			ImGui::Image(_gbufferDebugDescSets[1], ImVec2(64.0f, 64.0f));
			ImGui::SameLine();
			ImGui::Image(_gbufferDebugDescSets[2], ImVec2(64.0f, 64.0f));
			ImGui::SameLine();
			ImGui::Image(_gbufferDebugDescSets[3], ImVec2(64.0f, 64.0f));
			ImGui::SameLine();
			ImGui::Image(_gbufferDebugDescSets[4], ImVec2(64.0f, 64.0f));
			ImGui::TreePop();
		}
	}

	GBufferPass& GBufferPass::createAttachments(void)
	{
		_attachments.clear();
		_colorSampler.reset();

		const VkExtent3D attachmentExtent = { _gbufferResolution.width, _gbufferResolution.height, 1 };
		
		const VkSampleCountFlagBits maxSampleCount = VK_SAMPLE_COUNT_1_BIT; // getMaximumSampleCounts(_device);
		// 00. Diffuse
		_attachments.push_back({ createAttachment(attachmentExtent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, maxSampleCount) });

		// 01. Normal
		_attachments.push_back({ createAttachment(attachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, maxSampleCount) });

		// 02. Specular
		_attachments.push_back({ createAttachment(attachmentExtent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, maxSampleCount) });

		// 03. Emission
		_attachments.push_back({ createAttachment(attachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, maxSampleCount) });

		// 04. Tangent
		_attachments.push_back({ createAttachment(attachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, maxSampleCount) });

		// 05. Depth
		_attachments.push_back({ createAttachment(attachmentExtent, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, maxSampleCount)});

		// Create sampler for color attachments
		_colorSampler = std::make_shared<Sampler>(_device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_FILTER_LINEAR, 1.0f);

		// Share gbuffer contents with other render-passes
		_renderPassManager->put("DiffuseImageView",		_attachments[0].imageView.get());
		_renderPassManager->put("NormalImageView",		_attachments[1].imageView.get());
		_renderPassManager->put("SpecularImageView",	_attachments[2].imageView.get());
		_renderPassManager->put("EmissionImageView",	_attachments[3].imageView.get());
		_renderPassManager->put("TangentImageView",		_attachments[4].imageView.get());
		_renderPassManager->put("DepthImageView",		_attachments[5].imageView.get());
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
		_debugUtils.setObjectName(_attachments[4].image->getImageHandle(),			"GBuffer(Tangent)"		);
		_debugUtils.setObjectName(_attachments[4].imageView->getImageViewHandle(),	"GBuffer(Tangent) View");
		_debugUtils.setObjectName(_attachments[5].image->getImageHandle(),			"GBuffer(Depth)"		);
		_debugUtils.setObjectName(_attachments[5].imageView->getImageViewHandle(),	"GBuffer(Depth) View"	);
		_debugUtils.setObjectName(_colorSampler->getSamplerHandle(),				"GBuffer Sampler"		);
#endif

		return *this;
	}

	GBufferPass& GBufferPass::createRenderPass(void)
	{
		assert(_attachments.empty() == false); // snowapril : Attachments must be filled first
		_renderPass.reset();

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
			if (i == _attachments.size() - 1)
			{
				tempDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
			else
			{
				tempDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				colorAttachmentRefs.push_back({ attachmentOffset++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
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
		
		subpassDependencies[1].srcSubpass		= 0;
		subpassDependencies[1].dstSubpass		= VK_SUBPASS_EXTERNAL;
		subpassDependencies[1].srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[1].srcAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[1].dstStageMask		= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[1].dstAccessMask	= VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[1].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachmentRef.attachment	= attachmentOffset++;

		VkSubpassDescription subpassDesc = {};
		subpassDesc.colorAttachmentCount	= static_cast<uint32_t>(colorAttachmentRefs.size());
		subpassDesc.pColorAttachments		= colorAttachmentRefs.data();
		subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;
		subpassDesc.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;

		_renderPass = std::make_shared<RenderPass>();
		assert(_renderPass->initialize(_device, attachmentDesc, subpassDependencies, { subpassDesc }));
		return *this;
	}

	GBufferPass& GBufferPass::createFramebuffer(void)
	{
		assert(_attachments.empty() == false && _renderPass != nullptr);
		_framebuffer.reset();

		std::vector<VkImageView> imageViews(_attachments.size());
		for (size_t i = 0; i < _attachments.size(); ++i)
		{
			imageViews[i] = _attachments[i].imageView->getImageViewHandle();
		}
		_framebuffer = std::make_shared<Framebuffer>(_device, imageViews, _renderPass->getHandle(), _gbufferResolution);
		return *this;
	}

	GBufferPass& GBufferPass::createPipeline(const DescriptorSetLayoutPtr& globalDescLayout)
	{
		assert(_renderPass != nullptr && _attachments.empty() == false);
		SceneManager* sceneManager = _renderPassManager->get<SceneManager>("SceneManager");

		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(
			_device, 
			{ globalDescLayout, sceneManager->getDescriptorLayout() }, 
			{ sceneManager->getDefaultPushConstant() }
		);

		PipelineConfig config;
		PipelineConfig::GetDefaultConfig(&config);
		
		const std::vector<VkVertexInputBindingDescription>& bindingDesc		= sceneManager->getVertexInputBindingDesc(0);
		const std::vector<VkVertexInputAttributeDescription>& attribDescs	= sceneManager->getVertexInputAttribDesc(0);
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
		_pipeline->attachShaderModule(VK_SHADER_STAGE_VERTEX_BIT,	"Shaders/gBufferPass.vert.spv", nullptr);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/gBufferPass.frag.spv", nullptr);
		_pipeline->createPipeline(&config);
		return *this;
	}

	void GBufferPass::processWindowResize(int width, int height) 
	{
		// TODO(snowapril) : another pass that use current gbuffer attachments must be recreated too
		_gbufferResolution = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
		// vkDeviceWaitIdle(_device->getDeviceHandle());

		// createAttachments();
		// createRenderPass();
		// createFramebuffer();
	}
};