// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <RenderPass/RadianceInjectionPass.h>
#include <RenderPass/Voxelizer.h>
#include <RenderPass/RenderPassManager.h>
#include <RenderPass/DownSampler.h>
#include <VulkanFramework/FrameLayout.h>
#include <VulkanFramework/Image.h>
#include <VulkanFramework/ImageView.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Sampler.h>
#include <VulkanFramework/GraphicsPipeline.h>
#include <VulkanFramework/PipelineConfig.h>
#include <VulkanFramework/PipelineLayout.h>
#include <VulkanFramework/DescriptorSetLayout.h>
#include <VulkanFramework/DescriptorSet.h>
#include <VulkanFramework/DescriptorPool.h>
#include <VulkanFramework/Framebuffer.h>
#include <VulkanFramework/RenderPass.h>
#include <VulkanFramework/CommandBuffer.h>
#include <VulkanFramework/Utils.h>
#include <ClipmapRegion.h>
#include <DirectionalLight.h>
#include <GLTFScene.h>

namespace vfs
{
	RadianceInjectionPass::RadianceInjectionPass(DevicePtr device, uint32_t voxelResolution)
		: RenderPassBase(device),
		  _sampleCount(getMaximumSampleCounts(device)),
		  _voxelResolution(voxelResolution)
	{
		// Do nothing
	}

	RadianceInjectionPass::~RadianceInjectionPass()
	{
		// Do nothing
	}

	RadianceInjectionPass& RadianceInjectionPass::initialize(void)
	{
		_voxelizer				= _renderPassManager->get<Voxelizer>("Voxelizer"		);
		_voxelRadiance			= _renderPassManager->get<Image>	("VoxelRadiance"	);
		_voxelRadianceR32View	= _renderPassManager->get<ImageView>("VoxelRadianceR32View");
		_voxelSampler			= _renderPassManager->get<Sampler>	("VoxelSampler"		);
		return *this;
	}

	void RadianceInjectionPass::onBeginRenderPass(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
		cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, {}, {},
			{ _voxelRadiance->generateMemoryBarrier(0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
						VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL) }
		);

		_voxelizer->beginRenderPass(frameLayout);
	}

	void RadianceInjectionPass::onEndRenderPass(const FrameLayout* frameLayout)
	{
		_voxelizer->endRenderPass(frameLayout);

		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
		VkImageMemoryBarrier barrier = _voxelRadiance->generateMemoryBarrier(
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, {}, {}, { barrier });
	}

	void RadianceInjectionPass::onUpdate(const FrameLayout* frameLayout)
	{
		std::vector<ClipmapRegion>* clipRegions = _renderPassManager->get<std::vector<ClipmapRegion>>("ClipmapRegions");
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);

		// TODO(snowapril) : cached clip regions update
		constexpr uint32_t clipLevelToUpdate[] = { 0, 1, 2, 3, 4, 5 }; // TODO(snowapril) : remove this

		// 0. Radiance injection
		{
			DebugUtils::ScopedCmdLabel scope = _debugUtils.scopeLabel(cmdBuffer.getHandle(), "Radiance Voxelization");
			// TODO(snowapril) : get clip levels to update
			// clear radiance texture with that clip levels

			// TODO(snowapril) : Barrier need here 

			// TODO(snowapril) : bind directional light descriptor set
			// TODO(snowapril) : bind radiance texture descriptor set
			// TODO(snowapril) : call cmdVoxelize with clip levels to update
			cmdBuffer.bindPipeline(_pipeline);
			cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 2, {	_descriptorSet	  }, {});
			cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 4, { _lightDescriptorSet}, {});

			const std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>* clipmapRegions = _renderPassManager->get<std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>>("ClipmapRegions");
			std::vector<std::shared_ptr<GLTFScene>>* scenes = _renderPassManager->get<std::vector<std::shared_ptr<GLTFScene>>>("MainScenes");

			for (uint32_t clipLevel : clipLevelToUpdate)
			{
				// voxelize given region
				cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 3, { _voxelizer->getVoxelDescSet(clipLevel) }, {});
				_voxelizer->cmdVoxelize(cmdBuffer.getHandle(), clipmapRegions->at(clipLevel), clipLevel);
				for (std::shared_ptr<GLTFScene>& scene : *scenes)
				{
					scene->cmdDraw(cmdBuffer.getHandle(), _pipelineLayout, 0);
				}
			}
		}
		// TODO(snowapril) : copy alpha
		// 1. Copy alpha
		{

		}
		// TODO(snowapril) : downsampling voxel radiance here
		// 2. Down-sampling
		{
			DebugUtils::ScopedCmdLabel scope = _debugUtils.scopeLabel(cmdBuffer.getHandle(), "Radiance DownSampling");
			DownSampler* downSampler = _renderPassManager->get<DownSampler>("DownSampler");
			for (uint32_t clipLevel : clipLevelToUpdate)
			{
				if (clipLevel > 0)
				{
					// downSampler->cmdDownSampleRadiance(_voxelOpacityView, _voxelSampler, &_clipmapRegions, clipLevel);
				}
			}
		}
	}

	void RadianceInjectionPass::drawUI(void)
	{
		// Print Radiance Injection Pass elapsed time
	}
	
	RadianceInjectionPass& RadianceInjectionPass::createDescriptors(void)
	{
		// Descriptors for voxel textures
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
		};
		_descriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 1, 0);

		_descriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0);
		_descriptorLayout->createDescriptorSetLayout(0);

		_descriptorSet = std::make_shared<DescriptorSet>(_device, _descriptorPool, _descriptorLayout, 1);

		VkDescriptorImageInfo voxelRadianceInfo = {};
		voxelRadianceInfo.sampler		= _voxelSampler->getSamplerHandle();
		voxelRadianceInfo.imageView		= _voxelRadianceR32View->getImageViewHandle();
		voxelRadianceInfo.imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
		_descriptorSet->updateImage({ voxelRadianceInfo }, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		_shadowSampler = std::make_shared<Sampler>(_device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_FILTER_LINEAR, 0.0f);

		// Descriptors for directional lights
		poolSizes = {
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
		};
		_lightDescriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 1, 0);

		_lightDescriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_lightDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,	0);
		_lightDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,			0);
		_lightDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,			0);
		_lightDescriptorLayout->createDescriptorSetLayout(0);

		_lightDescriptorSet = std::make_shared<DescriptorSet>(_device, _lightDescriptorPool, _lightDescriptorLayout, 1);

		DirectionalLight* dirLight = _renderPassManager->get<DirectionalLight>("DirectionalLight");
		VkDescriptorImageInfo shadowMapInfo = {};
		shadowMapInfo.sampler		= _shadowSampler->getSamplerHandle();
		shadowMapInfo.imageView		= dirLight->getShadowMapView()->getImageViewHandle();
		shadowMapInfo.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		_lightDescriptorSet->updateImage({ shadowMapInfo }, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		_lightDescriptorSet->updateUniformBuffer({ dirLight->getLightDescBuffer() }, 1, 1);
		// _lightDescriptorSet->updateUniformBuffer({ dirLight->getLightShadowDescBuffer() }, 2, 1);
		return *this;
	}
	
	RadianceInjectionPass& RadianceInjectionPass::createPipeline(const DescriptorSetLayoutPtr& globalDescLayout)
	{
		// TODO(snowapril) : get common descriptor layout and push constant 
		std::vector<std::shared_ptr<GLTFScene>>* scenes = _renderPassManager->get<std::vector<std::shared_ptr<GLTFScene>>>("MainScenes");
		assert(scenes->empty() == false);
		std::shared_ptr<GLTFScene> gltfscene = scenes->at(0);

		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(
			_device, 
			{ globalDescLayout, gltfscene->getDescriptorLayout(), _descriptorLayout, _voxelizer->getVoxelDescLayout(), _lightDescriptorLayout },
			{ gltfscene->getDefaultPushConstant() }
		);

		PipelineConfig config;
		PipelineConfig::GetDefaultConfig(&config);
		
		const std::vector<VkVertexInputBindingDescription>& bindingDesc		= gltfscene->getVertexInputBindingDesc(0);
		const std::vector<VkVertexInputAttributeDescription>& attribDescs	= gltfscene->getVertexInputAttribDesc(0);
		config.vertexInputInfo.vertexAttributeDescriptionCount	= static_cast<uint32_t>(attribDescs.size());
		config.vertexInputInfo.pVertexAttributeDescriptions		= attribDescs.data();
		config.vertexInputInfo.vertexBindingDescriptionCount	= static_cast<uint32_t>(bindingDesc.size());
		config.vertexInputInfo.pVertexBindingDescriptions		= bindingDesc.data();
		config.inputAseemblyInfo.topology						= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		config.rasterizationInfo.cullMode						= VK_CULL_MODE_NONE;
		config.depthStencilInfo.depthTestEnable					= VK_FALSE;
		config.depthStencilInfo.stencilTestEnable				= VK_FALSE;
		config.renderPass										= _voxelizer->getRenderPass()->getHandle();
		config.pipelineLayout									= _pipelineLayout->getLayoutHandle();
		config.multisampleInfo.rasterizationSamples				= _sampleCount;
		if (_device->getDeviceFeature().sampleRateShading == VK_TRUE)
		{
			config.multisampleInfo.sampleShadingEnable = VK_TRUE;
			config.multisampleInfo.minSampleShading = 0.25f;
		}
		else
		{
			config.multisampleInfo.sampleShadingEnable = VK_FALSE;
		}
		config.dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		config.dynamicStateInfo.dynamicStateCount	= static_cast<uint32_t>(config.dynamicStates.size());
		config.dynamicStateInfo.pDynamicStates		= config.dynamicStates.data();
		config.viewportInfo.viewportCount			= 3;
		config.viewportInfo.scissorCount			= 3;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		colorBlendAttachment.colorWriteMask			= 0;
		colorBlendAttachment.blendEnable			= VK_FALSE;
		config.colorBlendInfo.attachmentCount		= 1;
		config.colorBlendInfo.pAttachments			= &colorBlendAttachment;

		_pipeline = std::make_shared<GraphicsPipeline>(_device);
		_pipeline->addShaderModule(VK_SHADER_STAGE_VERTEX_BIT,	 "Shaders/msaaInjectRadiance.vert.spv", nullptr);
		_pipeline->addShaderModule(VK_SHADER_STAGE_GEOMETRY_BIT, "Shaders/msaaInjectRadiance.geom.spv", nullptr);
		_pipeline->addShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/msaaInjectRadiance.frag.spv", nullptr);
		_pipeline->createPipeline(&config);

		return *this;
	}
};