// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <RenderPass/Clipmap/RadianceInjectionPass.h>
#include <RenderPass/Clipmap/Voxelizer.h>
#include <RenderPass/RenderPassManager.h>
#include <VulkanFramework/FrameLayout.h>
#include <VulkanFramework/Images/Image.h>
#include <VulkanFramework/Images/ImageView.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Images/Sampler.h>
#include <VulkanFramework/Pipelines/GraphicsPipeline.h>
#include <VulkanFramework/Pipelines/PipelineConfig.h>
#include <VulkanFramework/Pipelines/PipelineLayout.h>
#include <VulkanFramework/Descriptors/DescriptorSetLayout.h>
#include <VulkanFramework/Descriptors/DescriptorSet.h>
#include <VulkanFramework/Descriptors/DescriptorPool.h>
#include <VulkanFramework/RenderPass/Framebuffer.h>
#include <VulkanFramework/RenderPass/RenderPass.h>
#include <VulkanFramework/Commands/CommandBuffer.h>
#include <VulkanFramework/Utils.h>
#include <RenderPass/Clipmap/BorderWrapper.h>
#include <RenderPass/Clipmap/CopyAlpha.h>
#include <RenderPass/Clipmap/ClipmapRegion.h>
#include <RenderPass/Clipmap/ClipmapCleaner.h>
#include <RenderPass/Clipmap/DownSampler.h>
#include <DirectionalLight.h>
#include <SceneManager.h>

namespace vfs
{
	constexpr glm::vec2 kPoissonSamples[151] = {
#include <Resources/poisson151.data>
	};

	constexpr uint32_t kUpdateRegionLevelOffsets[DEFAULT_CLIP_REGION_COUNT]{
			1, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5
	};

	RadianceInjectionPass::RadianceInjectionPass(CommandPoolPtr cmdPool, uint32_t voxelResolution)
		: RenderPassBase(cmdPool),
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
		_voxelizer				= _renderPassManager->get<Voxelizer>("Voxelizer"			);
		_voxelRadiance			= _renderPassManager->get<Image>	("VoxelRadiance"		);
		_voxelOpacity			= _renderPassManager->get<Image>	("VoxelOpacity"			);
		_voxelRadianceView		= _renderPassManager->get<ImageView>("VoxelRadianceView"	);
		_voxelOpacityView		= _renderPassManager->get<ImageView>("VoxelOpacityView"		);
		_voxelRadianceR32View	= _renderPassManager->get<ImageView>("VoxelRadianceR32View"	);
		_voxelSampler			= _renderPassManager->get<Sampler>	("VoxelSampler"			);
		return *this;
	}

	void RadianceInjectionPass::onBeginRenderPass(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);

		// Clear revoxelization target regions
		{
			DebugUtils::ScopedCmdLabel scope = _debugUtils.scopeLabel(cmdBuffer.getHandle(), "Radiance Clear Regions");
			ClipmapCleaner* clipmapCleaner = _renderPassManager->get<ClipmapCleaner>("ClipmapCleaner");
			const std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>* clipmapRegions = _renderPassManager->get<std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>>("ClipmapRegions");
			for (uint32_t clipLevel = 0; clipLevel < DEFAULT_CLIP_REGION_COUNT; ++clipLevel)
			{
				if (_frameIndex % kUpdateRegionLevelOffsets[clipLevel] == 0)
				{
					clipmapCleaner->cmdClearRadianceClipRegion(cmdBuffer, _voxelRadiance, glm::ivec3(0), 
															   glm::uvec3(DEFAULT_VOXEL_RESOLUTION), clipLevel);
				}
			}
		}

		cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, {}, {},
			{ _voxelOpacity->generateMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
													VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL) }
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

		// 1. Copy Alpha
		{
			DebugUtils::ScopedCmdLabel scope = _debugUtils.scopeLabel(cmdBuffer.getHandle(), "Radiance Copy Alpha");
			CopyAlpha* copyAlpha = _renderPassManager->get<CopyAlpha>("CopyAlpha");
			for (uint32_t clipLevel = 0; clipLevel < DEFAULT_CLIP_REGION_COUNT; ++clipLevel)
			{
				if (_frameIndex % kUpdateRegionLevelOffsets[clipLevel] == 0)
				{
					copyAlpha->cmdImageCopyAlpha(cmdBuffer, _voxelRadiance, _voxelOpacity, clipLevel);
				}
			}
		}

		// 2. Down-sampling
		{
			DebugUtils::ScopedCmdLabel scope = _debugUtils.scopeLabel(cmdBuffer.getHandle(), "Radiance DownSampling");
			DownSampler* downSampler = _renderPassManager->get<DownSampler>("DownSampler");
			const std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>* clipmapRegions = _renderPassManager->get<std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>>("ClipmapRegions");
			for (uint32_t clipLevel = 1; clipLevel < DEFAULT_CLIP_REGION_COUNT; ++clipLevel)
			{
				if (_frameIndex % kUpdateRegionLevelOffsets[clipLevel] == 0)
				{
					downSampler->cmdDownSampleRadiance(cmdBuffer, _voxelRadiance, *clipmapRegions, clipLevel);
				}
			}
		}

		_frameIndex = (_frameIndex + 1);
	}

	void RadianceInjectionPass::onUpdate(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);

		// 0. Radiance injection
		{
			DebugUtils::ScopedCmdLabel scope = _debugUtils.scopeLabel(cmdBuffer.getHandle(), "Radiance Voxelization");

			cmdBuffer.bindPipeline(_pipeline);
			cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 2, {	_descriptorSet	  }, {});
			cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 4, { _lightDescriptorSet}, {});

			const std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>* clipmapRegions = _renderPassManager->get<std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>>("ClipmapRegions");
			SceneManager* sceneManager = _renderPassManager->get<SceneManager>("SceneManager");

			for (uint32_t clipLevel = 0; clipLevel < DEFAULT_CLIP_REGION_COUNT; ++clipLevel)
			{
				if (_frameIndex % kUpdateRegionLevelOffsets[clipLevel] == 0)
				{
					cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 3, { _voxelizer->getVoxelDescSet(clipLevel) }, {});
					_voxelizer->cmdVoxelize(cmdBuffer.getHandle(), clipmapRegions->at(clipLevel), clipLevel);
					sceneManager->cmdDraw(cmdBuffer.getHandle(), _pipelineLayout, 0);
				}
			}
		}
	}

	void RadianceInjectionPass::drawGUI(void)
	{
		// Print Radiance Injection Pass elapsed time
	}
	
	RadianceInjectionPass& RadianceInjectionPass::createDescriptors(void)
	{
		// Descriptors for voxel textures
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
		};
		_descriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 1, 0);

		_descriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_descriptorLayout->createDescriptorSetLayout(0);

		_descriptorSet = std::make_shared<DescriptorSet>(_device, _descriptorPool, _descriptorLayout, 1);

		VkDescriptorImageInfo voxelImageInfo = {};
		voxelImageInfo.sampler		= _voxelSampler->getSamplerHandle();
		voxelImageInfo.imageView	= _voxelRadianceR32View->getImageViewHandle();
		voxelImageInfo.imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
		_descriptorSet->updateImage({ voxelImageInfo }, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		
		voxelImageInfo.imageView	= _voxelOpacityView->getImageViewHandle();
		_descriptorSet->updateImage({ voxelImageInfo }, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		_possionSampleBuffer = std::make_shared<Buffer>(
			_device->getMemoryAllocator(), sizeof(kPoissonSamples), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU
		);
		_possionSampleBuffer->uploadData(&kPoissonSamples[0], sizeof(kPoissonSamples));
		_descriptorSet->updateUniformBuffer({ _possionSampleBuffer }, 2, 1);

		_shadowSampler = std::make_shared<Sampler>(_device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_FILTER_LINEAR, 0.0f);

		// Descriptors for directional lights
		poolSizes = {
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
		};
		_lightDescriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 1, 0);

		_lightDescriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_lightDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_lightDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_lightDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_lightDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_lightDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 4, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,		  0);
		_lightDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 5, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,		  0);
		_lightDescriptorLayout->createDescriptorSetLayout(0);

		_lightDescriptorSet = std::make_shared<DescriptorSet>(_device, _lightDescriptorPool, _lightDescriptorLayout, 1);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.sampler		= _shadowSampler->getSamplerHandle();
		imageInfo.imageView		= _renderPassManager->get<ImageView>("RSMPosition")->getImageViewHandle();
		imageInfo.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		_lightDescriptorSet->updateImage({ imageInfo }, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		imageInfo.imageView = _renderPassManager->get<ImageView>("RSMNormal")->getImageViewHandle();
		_lightDescriptorSet->updateImage({ imageInfo }, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		imageInfo.imageView = _renderPassManager->get<ImageView>("RSMFlux")->getImageViewHandle();
		_lightDescriptorSet->updateImage({ imageInfo }, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		DirectionalLight* dirLight = _renderPassManager->get<DirectionalLight>("DirectionalLight");
		imageInfo.imageView		= dirLight->getShadowMapView()->getImageViewHandle();
		imageInfo.imageLayout	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		_lightDescriptorSet->updateImage({ imageInfo }, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		_lightDescriptorSet->updateUniformBuffer({ dirLight->getLightDescBuffer() }, 4, 1);
		_lightDescriptorSet->updateUniformBuffer({ dirLight->getViewProjectionBuffer() }, 5, 1);
		return *this;
	}
	
	RadianceInjectionPass& RadianceInjectionPass::createPipeline(const DescriptorSetLayoutPtr& globalDescLayout)
	{
		// TODO(snowapril) : get common descriptor layout and push constant 
		SceneManager* sceneManager = _renderPassManager->get<SceneManager>("SceneManager");

		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(
			_device, 
			{ globalDescLayout, sceneManager->getDescriptorLayout(), _descriptorLayout, _voxelizer->getVoxelDescLayout(), _lightDescriptorLayout },
			{ sceneManager->getDefaultPushConstant() }
		);

		PipelineConfig config;
		PipelineConfig::GetDefaultConfig(&config);
		
		const std::vector<VkVertexInputBindingDescription>&		bindingDesc	= sceneManager->getVertexInputBindingDesc(0);
		const std::vector<VkVertexInputAttributeDescription>&	attribDescs	= sceneManager->getVertexInputAttribDesc(0);
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
		config.multisampleInfo.rasterizationSamples				= getMaximumSampleCounts(_device);
		if (_device->getDeviceFeature().sampleRateShading == VK_TRUE)
		{
			config.multisampleInfo.sampleShadingEnable	= VK_TRUE;
			config.multisampleInfo.minSampleShading		= 0.25f;
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
		_pipeline->attachShaderModule(VK_SHADER_STAGE_VERTEX_BIT,	"Shaders/msaaInjectRadiance.vert.spv", nullptr);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_GEOMETRY_BIT, "Shaders/msaaInjectRadiance.geom.spv", nullptr);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/msaaInjectRadiance.frag.spv", nullptr);
		_pipeline->createPipeline(&config);

		return *this;
	}
};