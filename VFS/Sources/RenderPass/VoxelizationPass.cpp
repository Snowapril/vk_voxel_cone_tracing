// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <RenderPass/VoxelizationPass.h>
#include <RenderPass/Voxelizer.h>
#include <VulkanFramework/CommandBuffer.h>
#include <VulkanFramework/Buffer.h>
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
#include <Camera.h>
#include <VulkanFramework/FrameLayout.h>
#include <VulkanFramework/Utils.h>
#include <VoxelEngine/Counter.h>
#include <RenderPass/RenderPassManager.h>
#include <RenderPass/DownSampler.h>
#include <GLTFScene.h>

namespace vfs
{
	VoxelizationPass::VoxelizationPass(DevicePtr device,
									   uint32_t voxelResolution, std::shared_ptr<GLTFScene> scene)
		: RenderPassBase(device), 
		  _voxelResolution(voxelResolution), 
		  _scene(scene), 
		  _sampleCount(getMaximumSampleCounts(device))
	{
		// Do nothing
	}

	VoxelizationPass::~VoxelizationPass()
	{
		// Do nothing
	}

	VoxelizationPass& VoxelizationPass::initialize(void)
	{
		_voxelizer			= _renderPassManager->get<Voxelizer>("Voxelizer"		);
		_voxelOpacity		= _renderPassManager->get<Image>	("VoxelOpacity"		);
		_voxelOpacityView	= _renderPassManager->get<ImageView>("VoxelOpacityView"	);
		_voxelSampler		= _renderPassManager->get<Sampler>	("VoxelSampler"		);
		return *this;
	}

	void VoxelizationPass::onBeginRenderPass(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);

		// TODO(snowapril) : clear only region need to be updated
		// Clear Opacity voxel clipmap for all face and clipmaps
		cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, {}, {},
			{ _voxelOpacity->generateMemoryBarrier(0, 0, VK_IMAGE_ASPECT_COLOR_BIT,
													VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) }
		);

		VkClearColorValue clearValue;
		clearValue.float32[0] = 0.0f;
		clearValue.float32[1] = 0.0f;
		clearValue.float32[2] = 0.0f;
		clearValue.float32[3] = 0.0f;
		VkImageSubresourceRange imageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		vkCmdClearColorImage(cmdBuffer.getHandle(), _voxelOpacity->getImageHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			&clearValue, 1, &imageSubresourceRange);

		cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, {}, {},
			{ _voxelOpacity->generateMemoryBarrier(0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
													VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL) }
		);

		_voxelizer->beginRenderPass(frameLayout);
	}

	void VoxelizationPass::onEndRenderPass(const FrameLayout* frameLayout)
	{
		_voxelizer->endRenderPass(frameLayout);
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
		VkImageMemoryBarrier barrier = _voxelOpacity->generateMemoryBarrier(
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		
		cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, {}, {}, { barrier });

		// TODO(snowapril) : downsampling voxel opacity here
		// 1. Down-sampling
		{
			DebugUtils::ScopedCmdLabel scope = _debugUtils.scopeLabel(cmdBuffer.getHandle(), "Opacity DownSampling");
			DownSampler* downSampler = _renderPassManager->get<DownSampler>("DownSampler");
			for (uint32_t i = 1; i < DEFAULT_CLIP_REGION_COUNT; ++i)
			{
				if (_revoxelizationRegions[i].empty() == false)
				{
					//downSampler->cmdDownSampleOpacity(cmdBuffer, _voxelOpacity, _voxelOpacityView, _voxelSampler, _clipmapRegions, i);
				}
			}
		}
	}
	
	void VoxelizationPass::onUpdate(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
		
		// TODO(snowapril) : torodial addressing
		// at now, every frame update all clipmap
		for (uint32_t i = 0; i < DEFAULT_CLIP_REGION_COUNT; ++i)
		{
			_revoxelizationRegions[i].clear();
			_revoxelizationRegions[i].push_back(_clipmapRegions[i]);
		}

		for (uint32_t i = 0; i < DEFAULT_CLIP_REGION_COUNT; ++i)
		{
			for (const ClipmapRegion& region : _revoxelizationRegions[i])
			{
				// TODO(snowapril) : clear voxel opacity 6 face
			}
		}
		
		// 0. Opacity Voxelization
		{
			DebugUtils::ScopedCmdLabel scope = _debugUtils.scopeLabel(cmdBuffer.getHandle(), "Opacity Voxelization");
			
			cmdBuffer.bindPipeline(_pipeline);
			cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 2, { _descriptorSet }, {});
			std::vector<std::shared_ptr<GLTFScene>>* scenes = _renderPassManager->get<std::vector<std::shared_ptr<GLTFScene>>>("MainScenes");
			for (uint32_t i = 0; i < DEFAULT_CLIP_REGION_COUNT; ++i)
			{
				for (const ClipmapRegion& region : _revoxelizationRegions[i])
				{
					// voxelize given region
					cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 3, { _voxelizer->getVoxelDescSet(i) }, {});
					_voxelizer->cmdVoxelize(frameLayout->commandBuffer, region, i);
					for (std::shared_ptr<GLTFScene>& scene : *scenes)
					{	
						scene->cmdDraw(frameLayout->commandBuffer, _pipelineLayout, 0);
					}
				}
			}
		}
	}

	void VoxelizationPass::drawUI(void)
	{
		// Print Voxelization Pass elapsed time
	}

	VoxelizationPass& VoxelizationPass::createVoxelClipmap(uint32_t extentLevel0)
	{
		const uint32_t voxelHalfResolution = _voxelResolution >> 1;

		for (size_t i = 0; i < _clipmapRegions.size(); ++i)
		{
			_clipmapRegions[i].minCorner = -glm::ivec3(voxelHalfResolution);
			_clipmapRegions[i].extent = { _voxelResolution, _voxelResolution, _voxelResolution };
			_clipmapRegions[i].voxelSize = (extentLevel0 * (1 << i)) / static_cast<float>(_voxelResolution);
		}

		std::array<BoundingBox<glm::vec3>, DEFAULT_CLIP_REGION_COUNT>* clipRegionBoundingBox =
			_renderPassManager->get< std::array<BoundingBox<glm::vec3>, DEFAULT_CLIP_REGION_COUNT>>("ClipRegionBoundingBox");
		
		for (uint32_t clipmapLevel = 0; clipmapLevel < DEFAULT_CLIP_REGION_COUNT; ++clipmapLevel)
		{
			const glm::ivec3 delta = calculateChangeDelta(clipmapLevel, clipRegionBoundingBox->at(clipmapLevel));
			_clipmapRegions[clipmapLevel].minCorner += delta;
		}

		_renderPassManager->put("ClipmapRegions", &_clipmapRegions);
		return *this;
	}

	VoxelizationPass& VoxelizationPass::createDescriptors(void)
	{
		// TODO(snowapril) : need to remove this
		_counter = std::make_unique<Counter>(_device);
		_counter->initialize(_device);
		_imageCoordBuffer = std::make_shared<Buffer>(_device->getMemoryAllocator(), sizeof(glm::ivec3) * 10000, 
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);
		_debugUtils.setObjectName(_imageCoordBuffer->getBufferHandle(), "ImageCoordBuffer");

		// Descriptors for voxel textures
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
		};
		_descriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 1, 0);

		_descriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0);
		// TODO(snowapril) : need to remove this
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0);
		_descriptorLayout->createDescriptorSetLayout(0);

		_descriptorSet = std::make_shared<DescriptorSet>(_device, _descriptorPool, _descriptorLayout, 1);

		VkDescriptorImageInfo voxelOpacityInfo = {};
		voxelOpacityInfo.sampler		= _voxelSampler->getSamplerHandle();
		voxelOpacityInfo.imageView		= _voxelOpacityView->getImageViewHandle();
		voxelOpacityInfo.imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
		_descriptorSet->updateImage({ voxelOpacityInfo }, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		_descriptorSet->updateStorageBuffer({ _counter->getBuffer() }, 1, 1);
		_descriptorSet->updateStorageBuffer({ _imageCoordBuffer }, 2, 1);

		return *this;
	}

	VoxelizationPass& VoxelizationPass::createPipeline(const DescriptorSetLayoutPtr& globalDescLayout)
	{
		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(
			_device,
			{ globalDescLayout, _scene->getDescriptorLayout(), _descriptorLayout, _voxelizer->getVoxelDescLayout() }, 
			{ _scene->getDefaultPushConstant() }
		);

		PipelineConfig config;
		PipelineConfig::GetDefaultConfig(&config);
		
		const std::vector<VkVertexInputBindingDescription>& bindingDesc = _scene->getVertexInputBindingDesc(0);
		const std::vector<VkVertexInputAttributeDescription>& attribDescs = _scene->getVertexInputAttribDesc(0);
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
		_pipeline->addShaderModule(VK_SHADER_STAGE_VERTEX_BIT,	 "Shaders/msaaVoxelizer.vert.spv", nullptr);
		_pipeline->addShaderModule(VK_SHADER_STAGE_GEOMETRY_BIT, "Shaders/msaaVoxelizer.geom.spv", nullptr);
		_pipeline->addShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/msaaVoxelizer.frag.spv", nullptr);
		_pipeline->createPipeline(&config);

		return *this;
	}

	glm::ivec3 VoxelizationPass::calculateChangeDelta(const uint32_t clipLevel, const BoundingBox<glm::vec3>& cameraBB)
	{
		assert(clipLevel < DEFAULT_CLIP_REGION_COUNT);
		const ClipmapRegion& region = _clipmapRegions[clipLevel];
		const float voxelSize = region.voxelSize;

		glm::vec3 deltaW = cameraBB.getMinCorner() - (glm::vec3(region.minCorner) * voxelSize);
		const float minChange = voxelSize * _clipMinChange[clipLevel];

		return glm::ivec3(glm::trunc(deltaW / minChange)) * _clipMinChange[clipLevel];
	}
};