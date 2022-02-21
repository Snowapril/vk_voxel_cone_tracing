// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <Util/EngineConfig.h>
#include <RenderPass/FinalPass.h>
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
#include <VulkanFramework/FrameLayout.h>
#include <VulkanFramework/Utils.h>
#include <DirectionalLight.h>
#include <RenderPass/Clipmap/ClipmapRegion.h>
#include <imgui/imgui.h>

namespace vfs
{
	FinalPass::FinalPass(CommandPoolPtr cmdPool,
						 RenderPassPtr renderPass)
	: RenderPassBase(cmdPool, renderPass)
	{
		// Do nothing
	}

	FinalPass::~FinalPass()
	{
		// Do nothing
	}

	void FinalPass::onUpdate(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);

		cmdBuffer.bindPipeline(_pipeline);
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 0, { _descriptorSet }, {});
		cmdBuffer.draw(4, 1, 0, 0);
	}

	FinalPass& FinalPass::createDescriptors(void)
	{
		// Descriptors for voxel cone tracing
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
		};
		_descriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 1, 0);

		_descriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_descriptorLayout->createDescriptorSetLayout(0);
		
		_descriptorSet = std::make_shared<DescriptorSet>(_device, _descriptorPool, _descriptorLayout, 1);
		
		VkDescriptorImageInfo descImageInfo = {};
		descImageInfo.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descImageInfo.imageView		= _renderPassManager->get<ImageView>("FinalOutputImageView")->getImageViewHandle();
		descImageInfo.sampler		= _renderPassManager->get<Sampler>("FinalOutputSampler")->getSamplerHandle();
		_descriptorSet->updateImage({ descImageInfo }, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		return *this;
	}

	FinalPass& FinalPass::createPipeline(void)
	{
		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(
			_device,
			{ _descriptorLayout },
			{ }
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

		config.multisampleInfo.rasterizationSamples = getMaximumSampleCounts(_device);
		if (_device->getDeviceFeature().sampleRateShading == VK_TRUE)
		{
			config.multisampleInfo.sampleShadingEnable = VK_TRUE;
			config.multisampleInfo.minSampleShading = 0.25f;
		}
		else
		{
			config.multisampleInfo.sampleShadingEnable = VK_FALSE;
		}

		_pipeline = std::make_shared<GraphicsPipeline>(_device);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_VERTEX_BIT,	"Shaders/finalPass.vert.spv", nullptr);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/finalPass.frag.spv", nullptr);
		_pipeline->createPipeline(&config);
		return *this;
	}
};