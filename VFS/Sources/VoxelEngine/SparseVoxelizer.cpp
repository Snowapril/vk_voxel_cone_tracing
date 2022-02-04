// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <Common/EngineConfig.h>
#include <VulkanFramework/CommandBuffer.h>
#include <VulkanFramework/CommandPool.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/Buffer.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/PipelineConfig.h>
#include <VulkanFramework/Fence.h>
#include <VulkanFramework/RenderPass.h>
#include <VulkanFramework/PipelineLayout.h>
#include <VulkanFramework/DescriptorPool.h>
#include <VulkanFramework/DescriptorSetLayout.h>
#include <VoxelEngine/SparseVoxelizer.h>
#include <VoxelEngine/Counter.h>
#include <GLTFScene.h>
#include <iostream>

namespace vfs
{
	SparseVoxelizer::SparseVoxelizer(std::shared_ptr<Device> device, std::shared_ptr<GLTFScene> scene, const uint32_t octreeLevel)
	{
		assert(initialize(device, scene, octreeLevel));
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

	bool SparseVoxelizer::initialize(std::shared_ptr<Device> device, std::shared_ptr<GLTFScene> scene, const uint32_t octreeLevel)
	{
		assert(octreeLevel <= MAX_OCTREE_LEVEL); // snowapril : octree level must be smaller than MAX_OCTREE_LEVEL
		_device			 = device;
		_scene			 = scene;
		_voxelResolution = 1 << octreeLevel;
		_level			 = octreeLevel;

		_counter = std::make_unique<Counter>();
		if (!_counter->initialize(_device))
		{
			std::cerr << "[VoxelEngine] Failed to create counter\n";
			return false;
		}

		if (!initializeRenderPass())
		{
			std::cerr << "[VoxelEngine] Failed to create render pass for voxelization pipeline\n";
			return false;
		}

		if (!initializeFramebuffer())
		{
			std::cerr << "[VoxelEngine] Failed to create framebuffer for voxelization pipeline\n";
			return false;
		}

		if (!initializeDescriptors())
		{
			std::cerr << "[VoxelEngine] Failed to create descriptors\n";
			return false;
		}

		if (!initializePipelineLayout())
		{
			std::cerr << "[VoxelEngine] Failed to create pipeline layout\n";
			return false;
		}

		if (!initializePipeline())
		{
			std::cerr << "[VoxelEngine] Failed to create voxelization pipeline\n";
			return false;
		}

		return true;
	}

	bool SparseVoxelizer::initializeRenderPass(void)
	{
		_renderPass = std::make_shared<RenderPass>();
		return _renderPass->initialize(_device, {}, {}, false);
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

	bool SparseVoxelizer::initializeDescriptors(void)
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2},
		};
		_descriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 2, 0);  // TODO(snowapril) : 2 is this good ?

		_descriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);;
		if (!_descriptorLayout->createDescriptorSetLayout(0))
		{
			return false;
		}

		_descriptorSet = std::make_shared<DescriptorSet>(_device, _descriptorPool, _descriptorLayout, 1);

		_descriptorSet->updateStorageBuffer({ _counter->getBuffer() }, 0, 1);
		return true;
	}

	bool SparseVoxelizer::initializePipelineLayout(void)
	{
		VkPushConstantRange scenePushConst = _scene->getDefaultPushConstant();
		VkPushConstantRange pushConstRange = {};
		pushConstRange.offset		= 0;
		pushConstRange.size			= sizeof(uint32_t) * 2 + scenePushConst.size;
		pushConstRange.stageFlags	= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		_pipelineLayout = std::make_shared<PipelineLayout>();
		return _pipelineLayout->initialize(_device, { _descriptorLayout, _scene->getDescriptorLayout() }, { pushConstRange });
	}

	bool SparseVoxelizer::initializePipeline(void)
	{
		assert(_pipelineLayout != VK_NULL_HANDLE); // snowapril : pipeline layout must be initialized first

		PipelineConfig config;
		PipelineConfig::GetDefaultConfig(&config);

		std::vector<VkVertexInputBindingDescription>	bindingDesc = _scene->getVertexInputBindingDesc(0);
		std::vector<VkVertexInputAttributeDescription>	attribDescs = _scene->getVertexInputAttribDesc(0);
		config.vertexInputInfo.vertexAttributeDescriptionCount	= static_cast<uint32_t>(attribDescs.size());
		config.vertexInputInfo.pVertexAttributeDescriptions		= attribDescs.data();
		config.vertexInputInfo.vertexBindingDescriptionCount	= static_cast<uint32_t>(bindingDesc.size());;
		config.vertexInputInfo.pVertexBindingDescriptions		= bindingDesc.data();
		config.inputAseemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		
		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		viewport.width  = static_cast<float>(_voxelResolution);
		viewport.height = static_cast<float>(_voxelResolution);
		config.viewportInfo.pViewports = &viewport;

		VkRect2D scissor = {};
		scissor.offset			= { 0, 0 };
		scissor.extent.width	= _voxelResolution;
		scissor.extent.height	= _voxelResolution;
		config.viewportInfo.pScissors = &scissor;

		config.renderPass		= _renderPass->getHandle();
		config.pipelineLayout	= _pipelineLayout->getLayoutHandle();
		
		config.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_8_BIT;
		// TODO(snowapril) : query sampleShaing support from device features

		_pipeline = std::make_shared<GraphicsPipeline>(_device);
		_pipeline->addShaderModule(VK_SHADER_STAGE_VERTEX_BIT,	 "Shaders/voxelizer.vert.spv", nullptr);
		_pipeline->addShaderModule(VK_SHADER_STAGE_GEOMETRY_BIT, "Shaders/voxelizer.geom.spv", nullptr);
		_pipeline->addShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/voxelizer.frag.spv", nullptr);
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

			VkViewport viewport;
			viewport.x = 0;
			viewport.y = 0;
			viewport.width = static_cast<float>(_voxelResolution);
			viewport.height = static_cast<float>(_voxelResolution);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			cmdBuffer.setViewport({ viewport });

			VkRect2D scissor;
			scissor.offset = { 0, 0 };
			scissor.extent = { _voxelResolution, _voxelResolution };
			cmdBuffer.setScissor({ scissor });

			cmdBuffer.bindPipeline(_pipeline);
			cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(),
				2, { _descriptorSet }, {});
			uint32_t pushValues[] = { _voxelResolution, 1 };
			cmdBuffer.pushConstants(_pipelineLayout->getLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushValues), pushValues);
			
			_scene->cmdDraw(cmdBuffer.getHandle(), _pipelineLayout, sizeof(pushValues));
			cmdBuffer.endRenderPass();
			cmdBuffer.endRecord();
		}

		Fence fence(_device, 1, 0);
		cmdPool->getQueue()->submitCmdBuffer({ cmdBuffer }, &fence);

		if (!fence.waitForAllFences(UINT64_MAX))
		{
			std::cerr << "[VoxelEngine] Failed to wait fence read counter value\n";
		}
		
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
		std::clog << "[VoxelEngine] Voxel fragment list : " << (_fragmentList->getTotalSize() / 1000) << " KB\n";
	}

	void SparseVoxelizer::cmdVoxelize(VkCommandBuffer cmdBuffer)
	{
		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = _renderPass->getHandle();
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = { _voxelResolution, _voxelResolution };
		renderPassBeginInfo.framebuffer = _framebuffer->getFramebufferHandle();
		renderPassBeginInfo.clearValueCount = 0;
		renderPassBeginInfo.pClearValues = nullptr;

		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport;
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = static_cast<float>(_voxelResolution);
		viewport.height = static_cast<float>(_voxelResolution);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor;
		scissor.offset = { 0, 0 };
		scissor.extent = { _voxelResolution, _voxelResolution };

		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		const VkDescriptorSet descSet = _descriptorSet->getHandle();
		_pipeline->bindPipeline(cmdBuffer);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 2, 1, &descSet, 0, nullptr);
		uint32_t pushValues[] = { _voxelResolution, 0 };
		vkCmdPushConstants(cmdBuffer, _pipelineLayout->getLayoutHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t) * 2, pushValues);
		_scene->cmdDraw(cmdBuffer, _pipelineLayout, sizeof(pushValues));

		vkCmdEndRenderPass(cmdBuffer);
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

		if (!fence.waitForAllFences(UINT64_MAX))
		{
			std::cerr << "[VoxelEngine] Failed to wait fence read counter value\n";
		}

		std::vector<glm::uvec2> downloadedData(_voxelFragmentCount);
		stagingBuffer->downloadData(downloadedData.data(), sizeof(glm::uvec2) * _voxelFragmentCount);
	}
}