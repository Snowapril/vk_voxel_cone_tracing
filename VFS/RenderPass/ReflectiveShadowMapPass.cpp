// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <RenderPass/ReflectiveShadowMapPass.h>
#include <RenderPass/RenderPassManager.h>
#include <VulkanFramework/Buffers/Buffer.h>
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
#include <imgui/imgui.h>
#include <Camera.h>
#include <SceneManager.h>

namespace vfs
{
	ReflectiveShadowMapPass::ReflectiveShadowMapPass(CommandPoolPtr cmdPool, VkExtent2D resolution)
		: RenderPassBase(cmdPool), _shadowMapResolution(resolution)
	{
		// Do nothing
	}

	ReflectiveShadowMapPass::~ReflectiveShadowMapPass()
	{
		// Do nothing
	}

	void ReflectiveShadowMapPass::onBeginRenderPass(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);

		std::vector<VkClearValue> clearValues(_attachments.size() + 1);
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
		viewport.width		= static_cast<float>(_shadowMapResolution.width);
		viewport.height		= static_cast<float>(_shadowMapResolution.height);
		cmdBuffer.setViewport({ viewport });

		VkRect2D scissor		= {};
		scissor.offset			= { 0, 0 };
		scissor.extent.width	= _shadowMapResolution.width;
		scissor.extent.height	= _shadowMapResolution.height;
		
		cmdBuffer.setScissor({ scissor });
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 2,
			{ _descriptorSet }, {});
	}

	void ReflectiveShadowMapPass::onEndRenderPass(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
		cmdBuffer.endRenderPass();
	}

	void ReflectiveShadowMapPass::onUpdate(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
	
		SceneManager* sceneManager = _renderPassManager->get<SceneManager>("SceneManager");
		sceneManager->cmdDraw(frameLayout->commandBuffer, _pipelineLayout, 0);
	}

	void ReflectiveShadowMapPass::drawGUI(void)
	{
		if (ImGui::TreeNode("Shadow Settings"))
		{
			_directionalLight->drawGUI();
			ImGui::TreePop();
		}
	}

	ReflectiveShadowMapPass& ReflectiveShadowMapPass::setDirectionalLight(std::unique_ptr<DirectionalLight>&& dirLight)
	{
		// Move framebuffer and directional light to proper storage
		_directionalLight = std::move(dirLight);
		_debugUtils.setObjectName(_directionalLight->getShadowMap()->getImageHandle(), "ShadowMap");
		_debugUtils.setObjectName(_directionalLight->getShadowMapView()->getImageViewHandle(), "ShadowMap View");

		// Create framebuffer for depth render target
		std::vector<VkImageView> imageViews;
		for (size_t i = 0; i < _attachments.size(); ++i)
		{
			imageViews.emplace_back(_attachments[i].imageView->getImageViewHandle());
		}
		imageViews.emplace_back(_directionalLight->getShadowMapView()->getImageViewHandle());
		_framebuffer = std::make_shared<Framebuffer>(_device, imageViews, _renderPass->getHandle(), _shadowMapResolution);

		// Descriptor set update
		_descriptorSet->updateUniformBuffer({ _directionalLight->getViewProjectionBuffer() }, 0, 1);
		_descriptorSet->updateUniformBuffer({ _directionalLight->getLightDescBuffer() }, 1, 1);

		_renderPassManager->put("DirectionalLight", _directionalLight.get());
		return *this;
	}

	ReflectiveShadowMapPass& ReflectiveShadowMapPass::createAttachments(void)
	{
		// TODO(snowapril) : manage light shadowmap from shadow map pass, not light itself
		const VkExtent3D attachmentExtent = { _shadowMapResolution.width, _shadowMapResolution.height, 1 };
		
		// 00. Position
		_attachments.push_back({createAttachment(attachmentExtent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)});

		// 01. Normal
		_attachments.push_back({createAttachment(attachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)});

		// 02. Flux
		_attachments.push_back({createAttachment(attachmentExtent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)});

		// 03. Depth
		// _attachments.push_back({createAttachment(attachmentExtent, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)});

		// Create sampler for color attachments
		_rsmSampler = std::make_shared<Sampler>(_device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_FILTER_LINEAR, 1.0f);

		// Share gbuffer contents with other render-passes
		_renderPassManager->put("RSMPosition",	_attachments[0].imageView.get());
		_renderPassManager->put("RSMNormal",	_attachments[1].imageView.get());
		_renderPassManager->put("RSMFlux",		_attachments[2].imageView.get());
		//_renderPassManager->put("RSMDepth",		_attachments[3].imageView.get());
		_renderPassManager->put("RSMSampler",	_rsmSampler.get());

#if defined(_DEBUG)
		_debugUtils.setObjectName(_attachments[0].image->getImageHandle(),			"RSM(Position)"		);
		_debugUtils.setObjectName(_attachments[0].imageView->getImageViewHandle(),	"RSM(Position) View"	);
		_debugUtils.setObjectName(_attachments[1].image->getImageHandle(),			"RSM(Normal)"		);
		_debugUtils.setObjectName(_attachments[1].imageView->getImageViewHandle(),	"RSM(Normal) View"	);
		_debugUtils.setObjectName(_attachments[2].image->getImageHandle(),			"RSM(Flux)"		);
		_debugUtils.setObjectName(_attachments[2].imageView->getImageViewHandle(),	"RSM(Flux) View");
		//_debugUtils.setObjectName(_attachments[3].image->getImageHandle(),			"RSM(Depth)"		);
		//_debugUtils.setObjectName(_attachments[3].imageView->getImageViewHandle(),	"RSM(Depth) View");
		_debugUtils.setObjectName(_rsmSampler->getSamplerHandle(),					"RSM Sampler"		);
#endif

		return *this;
	}

	ReflectiveShadowMapPass& ReflectiveShadowMapPass::createRenderPass(void)
	{
		assert(_attachments.empty() == false); // snowapril : Attachments must be filled first
		uint32_t attachmentOffset = 0;

		std::vector<VkAttachmentDescription> attachmentDesc;
		std::vector<VkAttachmentReference> colorAttachmentRefs;
		attachmentDesc.reserve(_attachments.size());
		colorAttachmentRefs.reserve(_attachments.size());
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
			tempDesc.finalLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			attachmentDesc.emplace_back(std::move(tempDesc));
			colorAttachmentRefs.push_back({ attachmentOffset++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		}
		
		VkAttachmentDescription tempDesc = {};
		tempDesc.format			= VK_FORMAT_D32_SFLOAT;
		tempDesc.samples		= VK_SAMPLE_COUNT_1_BIT;
		tempDesc.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
		tempDesc.storeOp		= VK_ATTACHMENT_STORE_OP_STORE;
		tempDesc.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		tempDesc.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		tempDesc.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		tempDesc.finalLayout	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		attachmentDesc.emplace_back(std::move(tempDesc));

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

	ReflectiveShadowMapPass& ReflectiveShadowMapPass::createPipeline(const DescriptorSetLayoutPtr& globalDescLayout)
	{
		assert(_renderPass != nullptr && _attachments.empty() == false);
		SceneManager* sceneManager = _renderPassManager->get<SceneManager>("SceneManager");

		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2}
		};
		_descriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 1, 0);

		_descriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_VERTEX_BIT,   0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_descriptorLayout->createDescriptorSetLayout(0);

		_descriptorSet = std::make_shared<DescriptorSet>(_device, _descriptorPool, _descriptorLayout, 1);

		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(
			_device, 
			{ globalDescLayout, sceneManager->getDescriptorLayout(), _descriptorLayout }, 
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
		config.colorBlendAttachments.resize(_attachments.size(), colorBlend);

		config.colorBlendInfo.attachmentCount = static_cast<uint32_t>(config.colorBlendAttachments.size());
		config.colorBlendInfo.pAttachments = config.colorBlendAttachments.data();
		
		_pipeline = std::make_shared<GraphicsPipeline>(_device);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_VERTEX_BIT,	"Shaders/reflectiveShadowPass.vert.spv", nullptr);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/reflectiveShadowPass.frag.spv", nullptr);
		_pipeline->createPipeline(&config);
		return *this;
	}
};