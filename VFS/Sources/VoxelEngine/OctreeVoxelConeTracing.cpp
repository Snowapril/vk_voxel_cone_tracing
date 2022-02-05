// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <Common/EngineConfig.h>
#include <RenderPass/RenderPassManager.h>
#include <VoxelEngine/OctreeVoxelConeTracing.h>
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
#include <ClipmapRegion.h>

namespace vfs
{
	OctreeVoxelConeTracing::OctreeVoxelConeTracing(DevicePtr device,
											   RenderPassPtr renderPass, 
											   VkExtent2D resolution)
	: RenderPassBase(device, renderPass), _resolution(resolution)
	{
		// Do nothing
	}

	OctreeVoxelConeTracing::~OctreeVoxelConeTracing()
	{
		// Do nothing
	}

	void OctreeVoxelConeTracing::onBeginRenderPass(const FrameLayout* frameLayout)
	{
		// Do nothing
	}

	void OctreeVoxelConeTracing::onEndRenderPass(const FrameLayout* frameLayout)
	{
		// Do nothing
	}

	void OctreeVoxelConeTracing::onUpdate(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);

		cmdBuffer.bindPipeline(_pipeline);
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 0, { frameLayout->globalDescSet }, {});
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 1, {		_gbufferDescriptorSet }, {});
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 2, {			   _descriptorSet }, {});
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 3, {		   _lightDescriptorSet}, {});
		cmdBuffer.pushConstants(_pipelineLayout->getLayoutHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &_renderingMode);
		cmdBuffer.draw(4, 1, 0, 0);
	}

	void OctreeVoxelConeTracing::drawUI(void)
	{
		// Print GBuffer Pass elapsed time
	}

	OctreeVoxelConeTracing& OctreeVoxelConeTracing::createDescriptors(void)
	{
		// Descriptors for voxel cone tracing
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,		10 },
		};
		_descriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 3, 0);

		_gbufferDescriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_gbufferDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_gbufferDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_gbufferDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_gbufferDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_gbufferDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 4, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_gbufferDescriptorLayout->createDescriptorSetLayout(0);
		
		_gbufferDescriptorSet = std::make_shared<DescriptorSet>(_device, _descriptorPool, _gbufferDescriptorLayout, 1);
		const ImageView* gbufferContents[] = {
			 _renderPassManager->get<ImageView>("DiffuseImageView"),
			 _renderPassManager->get<ImageView>("NormalImageView"),
			 _renderPassManager->get<ImageView>("SpecularImageView"),
			 _renderPassManager->get<ImageView>("EmissionImageView"),
			 _renderPassManager->get<ImageView>("DepthImageView")
		};
		const Sampler*	 gbufferSampler	= _renderPassManager->get<Sampler>("GBufferSampler");
		for (uint32_t i = 0; i < 5; ++i)
		{
			VkDescriptorImageInfo gbufferDescInfo = {};
			//gbufferDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			if (i == 4)
			{
				gbufferDescInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
			else
			{
				gbufferDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
			gbufferDescInfo.imageView	= gbufferContents[i]->getImageViewHandle();
			gbufferDescInfo.sampler		= gbufferSampler->getSamplerHandle();
			_gbufferDescriptorSet->updateImage({ gbufferDescInfo }, i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		}

		_descriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_descriptorLayout->createDescriptorSetLayout(0);

		_descriptorSet = std::make_shared<DescriptorSet>(_device, _descriptorPool, _descriptorLayout, 1);
		const ImageView*	voxelOpacityView	= _renderPassManager->get<ImageView>("VoxelOpacityView");
		const ImageView*	voxelRadianceView	= _renderPassManager->get<ImageView>("VoxelRadianceView");
		const Sampler*		voxelSampler		= _renderPassManager->get<Sampler>("VoxelSampler");

		VkDescriptorImageInfo voxelOpacityInfo = {};
		voxelOpacityInfo.sampler		= voxelSampler->getSamplerHandle();
		voxelOpacityInfo.imageView		= voxelOpacityView->getImageViewHandle();
		voxelOpacityInfo.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		_descriptorSet->updateImage({ voxelOpacityInfo }, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		VkDescriptorImageInfo voxelRadianceInfo = {};
		voxelRadianceInfo.sampler		= voxelSampler->getSamplerHandle();
		voxelRadianceInfo.imageView		= voxelRadianceView->getImageViewHandle();
		voxelRadianceInfo.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		_descriptorSet->updateImage({ voxelRadianceInfo }, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		
		_vctDescBuffer = std::make_shared<Buffer>(_device->getMemoryAllocator(), sizeof(VoxelConeTracingDesc), 
												  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>* clipmapRegions = 
			_renderPassManager->get<std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>>("ClipmapRegions");
		assert(clipmapRegions != nullptr);
		const ClipmapRegion& clipmapLevel0 = clipmapRegions->at(0);
		VoxelConeTracingDesc desc = {};
		desc.volumeCenter		= (glm::vec3(clipmapLevel0.minCorner) * clipmapLevel0.voxelSize) *
								  (glm::vec3(clipmapLevel0.extent) * clipmapLevel0.voxelSize) * 0.5f;
		desc.volumeDimension	= DEFAULT_VOXEL_RESOLUTION;
		desc.voxelSize			= clipmapLevel0.voxelSize;
		_vctDescBuffer->uploadData(&desc, sizeof(desc));
		_descriptorSet->updateUniformBuffer({ _vctDescBuffer }, 2, 1);

		_lightDescriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_lightDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_lightDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_lightDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_lightDescriptorLayout->createDescriptorSetLayout(0);

		_lightDescriptorSet = std::make_shared<DescriptorSet>(_device, _descriptorPool, _lightDescriptorLayout, 1);
		
		_shadowSampler = std::make_shared<Sampler>(_device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_FILTER_LINEAR, 0.0f);

		DirectionalLight* dirLight = _renderPassManager->get<DirectionalLight>("DirectionalLight");
		VkDescriptorImageInfo shadowMapInfo = {};
		shadowMapInfo.sampler		= _shadowSampler->getSamplerHandle();
		shadowMapInfo.imageView		= dirLight->getShadowMapView()->getImageViewHandle();
		shadowMapInfo.imageLayout	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		_lightDescriptorSet->updateImage({ shadowMapInfo }, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		_lightDescriptorSet->updateUniformBuffer({ dirLight->getLightDescBuffer() }, 1, 1);
		_lightDescriptorSet->updateUniformBuffer({ dirLight->getViewProjectionBuffer() }, 2, 1);

		return *this;
	}

	OctreeVoxelConeTracing& OctreeVoxelConeTracing::createPipeline(const DescriptorSetLayoutPtr& globalDescLayout)
	{
		VkPushConstantRange renderModePush = {};
		renderModePush.stageFlags	= VK_SHADER_STAGE_FRAGMENT_BIT;
		renderModePush.size			= sizeof(uint32_t);
		renderModePush.offset		= 0;

		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(
			_device,
			{ globalDescLayout, _gbufferDescriptorLayout, _descriptorLayout, _lightDescriptorLayout },
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

		_pipeline = std::make_shared<GraphicsPipeline>(_device);
		_pipeline->addShaderModule(VK_SHADER_STAGE_VERTEX_BIT,	 "Shaders/voxelConeTracing.vert.spv", nullptr);
		_pipeline->addShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/voxelConeTracing.frag.spv", nullptr);
		_pipeline->createPipeline(&config);
		return *this;
	}
};