// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <Util/EngineConfig.h>
#include <Common/Logger.h>
#include <VulkanFramework/Commands/CommandBuffer.h>
#include <VulkanFramework/Commands/CommandPool.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/Images/Image.h>
#include <VulkanFramework/Images/ImageView.h>
#include <VulkanFramework/Images/Sampler.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Pipelines/PipelineConfig.h>
#include <VulkanFramework/Sync/Fence.h>
#include <VulkanFramework/RenderPass/RenderPass.h>
#include <VulkanFramework/Pipelines/PipelineLayout.h>
#include <VulkanFramework/Descriptors/DescriptorPool.h>
#include <VulkanFramework/Descriptors/DescriptorSetLayout.h>
#include <VulkanFramework/Utils.h>
#include <RenderPass/Octree/SparseVoxelizer.h>
#include <Counter.h>
#include <DirectionalLight.h>
#include <SceneManager.h>

namespace vfs
{
	SparseVoxelizer::SparseVoxelizer(std::shared_ptr<Device> device, std::shared_ptr<SceneManager> sceneManager,
									 DirectionalLight* light, const uint32_t octreeLevel)
	{
		assert(initialize(device, sceneManager, light, octreeLevel));
	}

	SparseVoxelizer::~SparseVoxelizer()
	{
		destroyVoxelizer();
	}

	void SparseVoxelizer::destroyVoxelizer(void)
	{
		_framebuffer.reset();
		_counter.reset();
		_device.reset();
	}

	bool SparseVoxelizer::initialize(std::shared_ptr<Device> device, std::shared_ptr<SceneManager> sceneManager,
									 DirectionalLight* light, const uint32_t octreeLevel)
	{
		assert(octreeLevel <= MAX_OCTREE_LEVEL); // snowapril : octree level must be smaller than MAX_OCTREE_LEVEL
		_device			 = device;
		_sceneManager	 = sceneManager;
		_voxelResolution = 1 << octreeLevel;
		_level			 = octreeLevel;
		_sampleCount	 = getMaximumSampleCounts(_device);

		_counter = std::make_unique<Counter>();
		if (!_counter->initialize(_device))
		{
			VFS_ERROR << "Failed to create counter";
			return false;
		}

		if (!initializeRenderPass())
		{
			VFS_ERROR << "Failed to create render pass for voxelization pipeline";
			return false;
		}

		if (!initializeFramebuffer())
		{
			VFS_ERROR << "Failed to create framebuffer for voxelization pipeline";
			return false;
		}

		if (!initializeDescriptors(light))
		{
			VFS_ERROR << "Failed to create descriptors";
			return false;
		}

		if (!initializePipelineLayout())
		{
			VFS_ERROR << "Failed to create pipeline layout";
			return false;
		}

		if (!initializePipeline())
		{
			VFS_ERROR << "Failed to create voxelization pipeline";
			return false;
		}

		return true;
	}

	bool SparseVoxelizer::initializeRenderPass(void)
	{
		VkSubpassDescription subpassDesc = {};
		subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		_renderPass = std::make_shared<RenderPass>();
		return _renderPass->initialize(_device, {}, {}, { subpassDesc });
	}

	bool SparseVoxelizer::initializeFramebuffer(void)
	{
		_framebuffer = std::make_shared<Framebuffer>();
		if (!_framebuffer->initialize(_device, {}, _renderPass->getHandle(), { _voxelResolution, _voxelResolution }))
		{
			return false;
		}
		return true;
	}

	bool SparseVoxelizer::initializeDescriptors(DirectionalLight* light)
	{
		// Voxel Fragment list descriptor set
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2},
		};
		_descriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 1, 0);

		_descriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
		_descriptorLayout->createDescriptorSetLayout(0);

		_descriptorSet = std::make_shared<DescriptorSet>(_device, _descriptorPool, _descriptorLayout, 1);
		_descriptorSet->updateStorageBuffer({ _counter->getBuffer() }, 0, 1);

		// Light descriptor set
		poolSizes = {
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2},
		};
		_lightDescriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 1, 0);

		_lightDescriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_lightDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_lightDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_lightDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_lightDescriptorLayout->createDescriptorSetLayout(0);

		_lightDescriptorSet = std::make_shared<DescriptorSet>(_device, _lightDescriptorPool, _lightDescriptorLayout, 1);

		_shadowSampler = std::make_shared<Sampler>(_device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_FILTER_LINEAR, 0.0f);

		VkDescriptorImageInfo shadowMapInfo = {};
		shadowMapInfo.sampler		= _shadowSampler->getSamplerHandle();
		shadowMapInfo.imageView		= light->getShadowMapView()->getImageViewHandle();
		shadowMapInfo.imageLayout	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		_lightDescriptorSet->updateImage({ shadowMapInfo }, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		_lightDescriptorSet->updateUniformBuffer({ light->getLightDescBuffer() }, 1, 1);
		_lightDescriptorSet->updateUniformBuffer({ light->getViewProjectionBuffer() }, 2, 1);

		// Voxelization Descriptor set
		_viewProjBuffer = std::make_shared<Buffer>(_device->getMemoryAllocator(), sizeof(glm::mat4) * 6,
													VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		_sceneDimBuffer = std::make_shared<Buffer>(_device->getMemoryAllocator(), sizeof(glm::vec3) * 2,
													VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		glm::vec3 uploadData[] = {
			_sceneManager->getSceneBoundingBox().getMinCorner(), _sceneManager->getSceneBoundingBox().getMaxCorner()
		};
		_sceneDimBuffer->uploadData(uploadData, sizeof(glm::vec3) * 2);

		poolSizes = {
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2},
		};
		_voxelDescPool = std::make_shared<DescriptorPool>(_device, poolSizes, 1, 0);

		_voxelDescLayout = std::make_shared<DescriptorSetLayout>(_device);
		_voxelDescLayout->addBinding(VK_SHADER_STAGE_GEOMETRY_BIT, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_voxelDescLayout->addBinding(VK_SHADER_STAGE_VERTEX_BIT, 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_voxelDescLayout->createDescriptorSetLayout(0);

		_voxelDescSet = std::make_shared<DescriptorSet>(_device, _voxelDescPool, _voxelDescLayout, 1);
		_voxelDescSet->updateUniformBuffer({ _viewProjBuffer }, 0, 1);
		_voxelDescSet->updateUniformBuffer({ _sceneDimBuffer }, 1, 1);

		return true;
	}

	bool SparseVoxelizer::initializePipelineLayout(void)
	{
		VkPushConstantRange scenePushConst = _sceneManager->getDefaultPushConstant();
		VkPushConstantRange pushConstRange = {};
		pushConstRange.offset		= 0;
		pushConstRange.size			= sizeof(uint32_t) * 2 + scenePushConst.size;
		pushConstRange.stageFlags	= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		_pipelineLayout = std::make_shared<PipelineLayout>();
		return _pipelineLayout->initialize(
			_device, 
			{ _descriptorLayout, _sceneManager->getDescriptorLayout(), _lightDescriptorLayout, _voxelDescLayout },
			{ pushConstRange }
		);
	}

	bool SparseVoxelizer::initializePipeline(void)
	{
		assert(_pipelineLayout != VK_NULL_HANDLE); // snowapril : pipeline layout must be initialized first

		PipelineConfig config;
		PipelineConfig::GetDefaultConfig(&config);

		const std::vector<VkVertexInputBindingDescription>&		bindingDesc	= _sceneManager->getVertexInputBindingDesc(0);
		const std::vector<VkVertexInputAttributeDescription>&	attribDescs	= _sceneManager->getVertexInputAttribDesc(0);
		config.vertexInputInfo.vertexAttributeDescriptionCount	= static_cast<uint32_t>(attribDescs.size());
		config.vertexInputInfo.pVertexAttributeDescriptions		= attribDescs.data();
		config.vertexInputInfo.vertexBindingDescriptionCount	= static_cast<uint32_t>(bindingDesc.size());
		config.vertexInputInfo.pVertexBindingDescriptions		= bindingDesc.data();
		config.inputAseemblyInfo.topology						= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		config.rasterizationInfo.cullMode						= VK_CULL_MODE_NONE;
		config.depthStencilInfo.depthTestEnable					= VK_FALSE;
		config.depthStencilInfo.stencilTestEnable				= VK_FALSE;
		config.renderPass										= _renderPass->getHandle();
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
		_pipeline->attachShaderModule(VK_SHADER_STAGE_VERTEX_BIT,	"Shaders/voxelizer.vert.spv", nullptr);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_GEOMETRY_BIT, "Shaders/voxelizer.geom.spv", nullptr);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/voxelizer.frag.spv", nullptr);
		_pipeline->createPipeline(&config);
		return true;
	}

	void SparseVoxelizer::preVoxelize(const CommandPoolPtr& cmdPool)
	{
		_counter->resetCounter(cmdPool);

		CommandBuffer cmdBuffer(cmdPool->allocateCommandBuffer());
		{
			cmdBuffer.beginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			cmdBuffer.beginRenderPass(_renderPass, _framebuffer, {});

			setViewport(cmdBuffer);
			setViewProj(cmdBuffer);

			cmdBuffer.bindPipeline(_pipeline);
			cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(),
				0, { _descriptorSet }, {});
			cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(),
				2, { _lightDescriptorSet }, {});
			cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(),
				3, { _voxelDescSet }, {});
			uint32_t pushValues[] = { _voxelResolution, 1 };
			cmdBuffer.pushConstants(_pipelineLayout->getLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushValues), pushValues);
			
			_sceneManager->cmdDraw(cmdBuffer.getHandle(), _pipelineLayout, sizeof(pushValues));
			cmdBuffer.endRenderPass();
			cmdBuffer.endRecord();
		}

		Fence fence(_device, 1, 0);
		cmdPool->getQueue()->submitCmdBuffer({ cmdBuffer }, &fence);
		fence.waitForAllFences(UINT64_MAX);
		
		_voxelFragmentCount = _counter->readCounterValue(cmdPool);
		_counter->resetCounter(cmdPool);

		// TODO(snowapril) : fragment list update
		_fragmentList = std::make_shared<Buffer>();
		_fragmentList->initialize(_device->getMemoryAllocator(), sizeof(uint32_t) * 2 * _voxelFragmentCount,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
		
		VkDescriptorBufferInfo fragmentBufferInfo = {};
		fragmentBufferInfo.buffer = _fragmentList->getBufferHandle();
		fragmentBufferInfo.offset = 0;
		fragmentBufferInfo.range = _fragmentList->getTotalSize();
		
		VkWriteDescriptorSet writeSet = {};
		writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSet.pNext = nullptr;
		writeSet.dstBinding = 1;
		writeSet.dstSet = _descriptorSet->getHandle();
		writeSet.descriptorCount = 1;
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writeSet.pBufferInfo = &fragmentBufferInfo;
		
		vkUpdateDescriptorSets(_device->getDeviceHandle(), 1, &writeSet, 0, nullptr);
		VFS_INFO << "Voxel fragment list : " << (_fragmentList->getTotalSize() / 1000) << " KB";
	}

	void SparseVoxelizer::cmdVoxelize(VkCommandBuffer cmdBufferHandle)
	{
		CommandBuffer cmdBuffer(cmdBufferHandle);

		cmdBuffer.beginRenderPass(_renderPass, _framebuffer, {});

		setViewport(cmdBuffer);
		setViewProj(cmdBuffer);

		cmdBuffer.bindPipeline(_pipeline);
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(),
			0, { _descriptorSet }, {});
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(),
			2, { _lightDescriptorSet }, {});
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(),
			3, { _voxelDescSet }, {});
		uint32_t pushValues[] = { _voxelResolution, 0 };
		cmdBuffer.pushConstants(_pipelineLayout->getLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t) * 2, pushValues);
		_sceneManager->cmdDraw(cmdBuffer.getHandle(), _pipelineLayout, sizeof(pushValues));

		cmdBuffer.endRenderPass();
	}

	void SparseVoxelizer::readVoxelFragments(const CommandPoolPtr& cmdPool, OUT std::vector<glm::uvec3>* readData)
	{
		BufferPtr stagingBuffer = std::make_shared<Buffer>(_device->getMemoryAllocator(), 
														   sizeof(uint32_t) * 2 * _voxelFragmentCount,
														   VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		CommandBuffer cmdBuffer(cmdPool->allocateCommandBuffer());
		{
			cmdBuffer.beginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			VkBufferCopy bufferCopyInfo = {};
			bufferCopyInfo.size = sizeof(uint32_t) * 2 * _voxelFragmentCount;
			bufferCopyInfo.srcOffset = 0;
			bufferCopyInfo.dstOffset = 0;
			cmdBuffer.copyBuffer(_fragmentList.get(), stagingBuffer, { bufferCopyInfo });

			cmdBuffer.endRecord();
		}

		Fence fence(_device, 1, 0);
		cmdPool->getQueue()->submitCmdBuffer({ cmdBuffer }, &fence);
		fence.waitForAllFences(UINT64_MAX);

		std::vector<glm::uvec2> downloadedData(_voxelFragmentCount);
		stagingBuffer->downloadData(downloadedData.data(), sizeof(glm::uvec2) * _voxelFragmentCount);
	}

	void SparseVoxelizer::setViewport(CommandBuffer cmdBuffer)
	{
		std::vector<VkViewport> viewports {
			{ 0.0f, 0.0f, static_cast<float>(_voxelResolution), static_cast<float>(_voxelResolution), 0.0f, 1.0f},
			{ 0.0f, 0.0f, static_cast<float>(_voxelResolution), static_cast<float>(_voxelResolution), 0.0f, 1.0f},
			{ 0.0f, 0.0f, static_cast<float>(_voxelResolution), static_cast<float>(_voxelResolution), 0.0f, 1.0f},
		};
		cmdBuffer.setViewport(viewports);

		std::vector<VkRect2D> scissors {
			{ {0, 0 }, { _voxelResolution, _voxelResolution } },
			{ {0, 0 }, { _voxelResolution, _voxelResolution } },
			{ {0, 0 }, { _voxelResolution, _voxelResolution } }
		};
		cmdBuffer.setScissor(scissors);
	}

	void SparseVoxelizer::setViewProj(CommandBuffer cmdBuffer)
	{
		const BoundingBox<glm::vec3> dim = _sceneManager->getSceneBoundingBox();

		const glm::vec3 region		= dim.getMaxCorner() - dim.getMinCorner();
		const glm::vec3 minCorner	= dim.getMinCorner();
		const glm::vec3 eye			= minCorner + glm::vec3(0.0f, 0.0f, region.z);
		
		glm::mat4 viewProj[6];
		viewProj[0] = glm::ortho(-region.z, region.z, -region.y, region.y, -region.x, region.x) *
					  glm::lookAt(eye, eye + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		viewProj[1] = glm::ortho(-region.x, region.x, -region.z, region.z, -region.y, region.y) *
					  glm::lookAt(eye, eye + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
		viewProj[2] = glm::ortho(-region.x, region.x, -region.y, region.y, -region.z, region.z) *
					  glm::lookAt(minCorner, minCorner + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		
		for (uint32_t i = 0; i < 3; ++i)
		{
			viewProj[i + 3] = glm::inverse(viewProj[i]);
		}
		
		_viewProjBuffer->uploadData(&viewProj[0], sizeof(glm::mat4) * 6);
	}
}