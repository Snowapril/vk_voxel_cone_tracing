// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <RenderPass/ShadowMapPass.h>
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
#include <Camera.h>
#include <VulkanFramework/FrameLayout.h>
#include <VulkanFramework/Buffers/Buffer.h>
#include <RenderPass/RenderPassManager.h>
#include <GLTFScene.h>

namespace vfs
{
	ShadowMapPass::ShadowMapPass(DevicePtr device, VkExtent2D resolution)
		: RenderPassBase(device), _shadowMapResolution(resolution)
	{
		// Do nothing
	}

	ShadowMapPass::~ShadowMapPass()
	{
		// Do nothing
	}

	void ShadowMapPass::onBeginRenderPass(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
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

		VkClearValue depthClear;
		depthClear.depthStencil = { 1.0f, 0 };
		cmdBuffer.beginRenderPass(_renderPass, _framebuffer, { depthClear });
	}

	void ShadowMapPass::onEndRenderPass(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
		cmdBuffer.endRenderPass();

		// VkImageMemoryBarrier barrier = _directionalLight->getShadowMap()->generateMemoryBarrier(
		// 	VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_ASPECT_DEPTH_BIT,
		// 	VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
		// );
		// cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_BOTTOM_)
	}

	void ShadowMapPass::onUpdate(const FrameLayout* frameLayout)
	{
		std::vector<std::shared_ptr<GLTFScene>>* scenes = 
			_renderPassManager->get<std::vector<std::shared_ptr<GLTFScene>>>("MainScenes");
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);

		for (std::shared_ptr<GLTFScene>& scene : *scenes)
		{
			scene->cmdDraw(frameLayout->commandBuffer, _pipelineLayout, 0);
		}
	}

	void ShadowMapPass::drawUI(void)
	{
		// Print Voxelization Pass elapsed time
	}

	ShadowMapPass& ShadowMapPass::setDirectionalLight(std::unique_ptr<DirectionalLight>&& dirLight)
	{
		// Move framebuffer and directional light to proper storage
		_directionalLight = std::move(dirLight);
		_debugUtils.setObjectName(_directionalLight->getShadowMap()->getImageHandle(), "ShadowMap");
		_debugUtils.setObjectName(_directionalLight->getShadowMapView()->getImageViewHandle(), "ShadowMap View");

		// Create framebuffer for depth render target
		std::vector<VkImageView> imageViews{ _directionalLight->getShadowMapView()->getImageViewHandle() };
		_framebuffer = std::make_shared<Framebuffer>(_device, imageViews, _renderPass->getHandle(), _shadowMapResolution);

		// Descriptor set update
		_descriptorSet->updateUniformBuffer({ _directionalLight->getViewProjectionBuffer() }, 0, 1);

		_renderPassManager->put("DirectionalLight", _directionalLight.get());
		return *this;
	}

	ShadowMapPass& ShadowMapPass::createRenderPass(void)
	{
		// TODO(snowapril) : check depth attachment format support
		VkAttachmentDescription depthDesc = {};
		depthDesc.format			= VK_FORMAT_D32_SFLOAT;
		depthDesc.samples			= VK_SAMPLE_COUNT_1_BIT;
		depthDesc.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthDesc.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
		depthDesc.stencilLoadOp		= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthDesc.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthDesc.initialLayout		= VK_IMAGE_LAYOUT_UNDEFINED;
		depthDesc.finalLayout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		std::vector<VkSubpassDependency> subpassDependencies(2, VkSubpassDependency{});
		subpassDependencies[0].srcSubpass		= VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].dstSubpass		= 0;
		subpassDependencies[0].srcStageMask		= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependencies[0].srcAccessMask	= VK_ACCESS_SHADER_READ_BIT;
		subpassDependencies[0].dstStageMask		= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpassDependencies[0].dstAccessMask	= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;
		
		subpassDependencies[1].srcSubpass		= 0;
		subpassDependencies[1].dstSubpass		= VK_SUBPASS_EXTERNAL;
		subpassDependencies[1].srcStageMask		= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		subpassDependencies[1].srcAccessMask	= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		subpassDependencies[1].dstStageMask		= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependencies[1].dstAccessMask	= VK_ACCESS_SHADER_READ_BIT;
		subpassDependencies[1].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;

		_renderPass = std::make_shared<RenderPass>();
		assert(_renderPass->initialize(_device, { depthDesc }, subpassDependencies, true));
		return *this;
	}

	ShadowMapPass& ShadowMapPass::createPipeline(const DescriptorSetLayoutPtr& globalDescLayout,
												 const GLTFScene* scene)
	{
		assert(_renderPass != nullptr);

		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}
		};
		_descriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 1, 0);

		_descriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_VERTEX_BIT, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_descriptorLayout->createDescriptorSetLayout(0);

		_descriptorSet = std::make_shared<DescriptorSet>(_device, _descriptorPool, _descriptorLayout, 1);

		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(_device, { globalDescLayout, scene->getDescriptorLayout(), _descriptorLayout }, { scene->getDefaultPushConstant() });

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
		
		_pipeline = std::make_shared<GraphicsPipeline>(_device);
		_pipeline->addShaderModule(VK_SHADER_STAGE_VERTEX_BIT,	 "Shaders/shadowPass.vert.spv", nullptr);
		_pipeline->addShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/shadowPass.frag.spv", nullptr);
		_pipeline->createPipeline(&config);
		return *this;
	}
};