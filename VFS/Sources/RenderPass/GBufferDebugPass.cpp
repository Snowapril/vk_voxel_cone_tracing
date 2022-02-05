// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <RenderPass/GBufferDebugPass.h>
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

namespace vfs
{
	GBufferDebugPass::GBufferDebugPass(vfs::DevicePtr device, const RenderPassPtr& renderPass)
	{
		assert(initialize(device, renderPass));
	}

	GBufferDebugPass::~GBufferDebugPass()
	{
		_pipelineLayout.reset();
		_pipeline.reset();
		_device.reset();
	}

	bool GBufferDebugPass::initialize(vfs::DevicePtr device, const RenderPassPtr& renderPass)
	{
		_device = device;

		if (!initializePipeline(renderPass))
		{
			return false;
		}
		return true;
	}

	void GBufferDebugPass::beginRenderPass(const FrameLayout* frameLayout, const CameraPtr& camera)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
		cmdBuffer.bindPipeline(_pipeline);
	}

	void GBufferDebugPass::endRenderPass(const FrameLayout* frameLayout)
	{
		(void)frameLayout;
	}
	
	DescriptorSetPtr GBufferDebugPass::createDescriptorSet(void)
	{
		return std::make_shared<DescriptorSet>(_device, _descriptorPool, _descriptorLayout, 1);
	}

	bool GBufferDebugPass::initializePipeline(const RenderPassPtr& renderPass)
	{
		// TODO(snowapril) : remove this hard coded descriptor pool
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
		};
		_descriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 4, 0);

		_descriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_descriptorLayout->createDescriptorSetLayout(0);

		VkPushConstantRange pushConst = {};
		pushConst.offset = 0;
		pushConst.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConst.size = sizeof(uint32_t);

		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(_device, { _descriptorLayout }, { pushConst });

		PipelineConfig config;
		PipelineConfig::GetDefaultConfig(&config);

		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		viewport.width = static_cast<float>(1280);
		viewport.height = static_cast<float>(920);
		config.viewportInfo.pViewports = &viewport;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent.width = 1280;
		scissor.extent.height = 920;
		config.viewportInfo.pScissors = &scissor;

		config.inputAseemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		config.renderPass = renderPass->getHandle();
		config.pipelineLayout = _pipelineLayout->getLayoutHandle();

		_pipeline = std::make_shared<GraphicsPipeline>(_device);
		_pipeline->addShaderModule(VK_SHADER_STAGE_VERTEX_BIT,	 "Shaders/quad.vert.spv", nullptr);
		_pipeline->addShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/quad.frag.spv", nullptr);
		_pipeline->createPipeline(&config);

		return true;
	}
};