// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <Common/EngineConfig.h>
#include <Common/Utils.h>
#include <VoxelEngine/OctreeBuilder.h>
#include <VoxelEngine/SparseVoxelizer.h>
#include <VoxelEngine/Counter.h>
#include <VulkanFramework/CommandBuffer.h>
#include <VulkanFramework/DescriptorPool.h>
#include <VulkanFramework/Fence.h>
#include <VulkanFramework/DescriptorSet.h>
#include <VulkanFramework/DescriptorSetLayout.h>
#include <VulkanFramework/PipelineConfig.h>
#include <VulkanFramework/PipelineLayout.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Buffer.h>
#include <VulkanFramework/BufferView.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/CommandPool.h>
#include <iostream>

namespace vfs
{
	OctreeBuilder::OctreeBuilder(DevicePtr device, const CommandPoolPtr& cmdPool, std::shared_ptr<SparseVoxelizer> voxelizer)
	{
		assert(initialize(device, cmdPool, voxelizer));
	}

	OctreeBuilder::~OctreeBuilder()
	{
		destroyOctreeBuilder();
	}

	void OctreeBuilder::destroyOctreeBuilder(void)
	{
		_pipelineLayout.reset();
		_descriptorSet.reset();
		_descriptorLayout.reset();
		_octreeBrickImageBuffer.reset();
		_device.reset();
		_nodeInitPipeline.reset();
		_nodeFlagPipeline.reset();
		_nodeAllocPipeline.reset();
		_nodeModifyArgPipeline.reset();
		_nodeLeafWritePipeline.reset();
		_nodeMipmapWritePipeline.reset();
		_infoBuffer.reset();
		_infoStagingBuffer.reset();
		_indirectBuffer.reset();
		_indirectStagingBuffer.reset();
		_octreeBuffer.reset();
		_octreeBrickCounter.reset();
		_octreeNodeCounter.reset();
		_voxelizer.reset();
	}

	bool OctreeBuilder::initialize(DevicePtr device, const CommandPoolPtr& cmdPool, std::shared_ptr<SparseVoxelizer> voxelizer)
	{
		_device		= device;
		_voxelizer	= voxelizer;
		_level		= _voxelizer->getLevel();

		if (!initializeBuffers(cmdPool))
		{
			return false;
		}

		if (!initializeDescriptors())
		{
			return false;
		}

		if (!initializePipelineLayout())
		{
			return false;
		}

		if (!initializePipelines())
		{
			return false;
		}
		return true;
	}

	bool OctreeBuilder::initializeBuffers(const CommandPoolPtr& cmdPool)
	{
		const VmaAllocator allocator = _device->getMemoryAllocator();
		
		_infoBuffer = std::make_shared<Buffer>(allocator, sizeof(uint32_t) * 2,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);
		_infoStagingBuffer = std::make_shared<Buffer>(allocator, sizeof(uint32_t) * 2,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY);

		// TODO(snowapril) : this hard-coded data
		uint32_t infoData[] = { 0, 8 };
		_infoStagingBuffer->uploadData(infoData, sizeof(infoData));

		_indirectBuffer = std::make_shared<Buffer>(allocator, sizeof(uint32_t) * 3,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);
		_indirectStagingBuffer = std::make_shared<Buffer>(allocator, sizeof(uint32_t) * 3,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY);

		// TODO(snowapril) : this hard-coded data
		uint32_t indirectData[] = { 1, 1, 1 };
		_indirectStagingBuffer->uploadData(indirectData, sizeof(indirectData));

		_numOctreeNodes = vfs::min(
			vfs::max(MIN_OCTREE_NODE_NUM, _voxelizer->getFragmentCount() << 3u),
			MAX_OCTREE_NODE_NUM
		);
		_octreeBuffer = std::make_shared<Buffer>(allocator, _numOctreeNodes * sizeof(glm::uvec2),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // TODO(snowapril) : remove transfer bit
			VMA_MEMORY_USAGE_GPU_ONLY);
		
		_numOctreeBricks = vfs::min(
			vfs::max(MIN_OCTREE_BRICK_NUM, _voxelizer->getFragmentCount() << 3u),
			MAX_OCTREE_BRICK_NUM
		);

		_octreeBrickImageBuffer = std::make_shared<Buffer>(allocator, _numOctreeBricks * sizeof(uint32_t),
			VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // TODO(snowapril) : remove transfer bit
			VMA_MEMORY_USAGE_GPU_ONLY);

		_octreeBrickImageBufferView = std::make_shared<BufferView>(_device, _octreeBrickImageBuffer, VK_FORMAT_R32_UINT);

		_octreeNodeCounter = std::make_shared<Counter>();
		if (!_octreeNodeCounter->initialize(_device))
		{
			std::cerr << "[VoxelEngine] Failed to create counter\n";
			return false;
		}

		// TODO(snowapril) : main command pool remove
		_octreeNodeCounter->resetCounter(cmdPool);

		_octreeBrickCounter = std::make_shared<Counter>();
		if (!_octreeBrickCounter->initialize(_device))
		{
			std::cerr << "[VoxelEngine] Failed to create counter\n";
			return false;
		}

		// TODO(snowapril) : main command pool remove
		_octreeBrickCounter->resetCounter(cmdPool);

		std::clog << "[VoxelEngine] Octree node   allocated : " << _numOctreeNodes / 1000 << " KB\n";
		std::clog << "[VoxelEngine] Octree bricks allocated : " << _numOctreeBricks / 1000 << " KB\n";

		return true;
	}

	bool OctreeBuilder::initializeDescriptors(void)
	{
		const std::vector<VkDescriptorPoolSize> poolSizes = {
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5},
			{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1}
		};
		_descriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 6, 0);

		_descriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,		  0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,		  0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,		  0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,		  0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,		  0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 5, 1, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 0);
		if (!_descriptorLayout->createDescriptorSetLayout(0))
		{
			return false;
		}

		_descriptorSet = std::make_shared<DescriptorSet>(_device, _descriptorPool, _descriptorLayout, 1);
		_descriptorSet->updateStorageBuffer({ _octreeNodeCounter->getBuffer() },  0, 1);
		_descriptorSet->updateStorageBuffer({ _octreeBuffer },					  1, 1);
		_descriptorSet->updateStorageBuffer({ _voxelizer->getFramgnetList() },	  2, 1);
		_descriptorSet->updateStorageBuffer({ _infoBuffer },					  3, 1);
		_descriptorSet->updateStorageBuffer({ _indirectBuffer },				  4, 1);
		_descriptorSet->updateStorageTexelBuffer(_octreeBrickImageBufferView, 5, 1);
		return true;
	}

	bool OctreeBuilder::initializePipelineLayout(void)
	{
		VkPushConstantRange pushConstRange = {};
		pushConstRange.offset = 0;
		pushConstRange.size = sizeof(uint32_t) * 3;
		pushConstRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		_pipelineLayout = std::make_shared<PipelineLayout>();
		return _pipelineLayout->initialize(_device, { _descriptorLayout }, { pushConstRange });
	}

	bool OctreeBuilder::initializePipelines(void)
	{
		assert(_pipelineLayout != VK_NULL_HANDLE); // snowapril : pipeline layout must be initialized first
		PipelineConfig config;
		config.pipelineLayout = _pipelineLayout->getLayoutHandle();

		_nodeInitPipeline = std::make_shared<ComputePipeline>();
		_nodeInitPipeline->initialize(_device);
		_nodeInitPipeline->addShaderModule(VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/octreeNodeInit.comp.spv", nullptr);
		_nodeInitPipeline->createPipeline(&config);

		_nodeFlagPipeline = std::make_shared<ComputePipeline>();
		_nodeFlagPipeline->initialize(_device);
		_nodeFlagPipeline->addShaderModule(VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/octreeNodeFlag.comp.spv", nullptr);
		_nodeFlagPipeline->createPipeline(&config);

		_nodeAllocPipeline = std::make_shared<ComputePipeline>();
		_nodeAllocPipeline->initialize(_device);
		_nodeAllocPipeline->addShaderModule(VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/octreeNodeAlloc.comp.spv", nullptr);
		_nodeAllocPipeline->createPipeline(&config);

		_nodeModifyArgPipeline = std::make_shared<ComputePipeline>();
		_nodeModifyArgPipeline->initialize(_device);
		_nodeModifyArgPipeline->addShaderModule(VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/octreeNodeModifyArg.comp.spv", nullptr);
		_nodeModifyArgPipeline->createPipeline(&config);

		_nodeLeafWritePipeline = std::make_shared<ComputePipeline>();
		_nodeLeafWritePipeline->initialize(_device);
		_nodeLeafWritePipeline->addShaderModule(VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/octreeNodeLeafWrite.comp.spv", nullptr);
		_nodeLeafWritePipeline->createPipeline(&config);

		_nodeMipmapWritePipeline = std::make_shared<ComputePipeline>	();
		_nodeMipmapWritePipeline->initialize(_device);
		_nodeMipmapWritePipeline->addShaderModule(VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/octreeNodeMipmapWrite.comp.spv", nullptr);
		_nodeMipmapWritePipeline->createPipeline(&config);

		return true;
	}

	void OctreeBuilder::cmdBuild(VkCommandBuffer cmdBuffer)
	{
		// Top-down building
		cmdNodeSubdivision(cmdBuffer);
		// Bottom-up mipmap write
		cmdMipmapWrite(cmdBuffer);
	}

	void OctreeBuilder::cmdNodeSubdivision(VkCommandBuffer cmdBufferHandle)
	{
		CommandBuffer cmdBuffer(cmdBufferHandle);
		{
			VkBufferCopy copyRegion = {};
			copyRegion.size = sizeof(uint32_t) * 2;
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			cmdBuffer.copyBuffer(_infoStagingBuffer.get(), _infoBuffer, { copyRegion });

			copyRegion.size = sizeof(uint32_t) * 3;
			cmdBuffer.copyBuffer(_indirectStagingBuffer.get(), _indirectBuffer, { copyRegion });

			VkBufferMemoryBarrier infoBufferBarrier = _infoBuffer->generateMemoryBarrier(
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
			);
			cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0, {}, { infoBufferBarrier }, {});
			VkBufferMemoryBarrier indirectBufferBarrier = _indirectBuffer->generateMemoryBarrier(
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
			);
			cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0, {}, { indirectBufferBarrier }, {});
		}
		
		const uint32_t numVoxelFragment = _voxelizer->getFragmentCount();
		const uint32_t fragmentGroupX = numVoxelFragment / 64 + ((numVoxelFragment & 0x3f) > 0 ? 1 : 0);
		uint32_t pushValues[] = { numVoxelFragment, _voxelizer->getVoxelResolution() };
		cmdBuffer.pushConstants(_pipelineLayout->getLayoutHandle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushValues), pushValues);
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, _pipelineLayout->getLayoutHandle(), 0, { _descriptorSet }, {});

		for (uint32_t i = 1; i <= _level; ++i)
		{
			cmdBuffer.bindPipeline(_nodeInitPipeline);
			cmdBuffer.dispatchIndirect(_indirectBuffer, 0);

			VkBufferMemoryBarrier octreeBufferBarrier = _octreeBuffer->generateMemoryBarrier(
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
			);
			cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0, {}, { octreeBufferBarrier }, {});

			cmdBuffer.bindPipeline(_nodeFlagPipeline);
			cmdBuffer.dispatch(fragmentGroupX, 1, 1);

			if (i != _level)
			{
				octreeBufferBarrier = _octreeBuffer->generateMemoryBarrier(
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
				);
				cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0, {}, { octreeBufferBarrier }, {});

				cmdBuffer.bindPipeline(_nodeAllocPipeline);
				cmdBuffer.dispatchIndirect(_indirectBuffer, 0);

				octreeBufferBarrier = _octreeBuffer->generateMemoryBarrier(
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
				);
				cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0, {}, { octreeBufferBarrier }, {});

				cmdBuffer.bindPipeline(_nodeModifyArgPipeline);
				cmdBuffer.dispatch(1, 1, 1);

				VkBufferMemoryBarrier indirectBufferBarrier = _indirectBuffer->generateMemoryBarrier(
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
				);
				cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0, {}, { indirectBufferBarrier }, {});

				VkBufferMemoryBarrier buildInfoBufferBarrier = _infoBuffer->generateMemoryBarrier(
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT
				);
				cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0, {}, { buildInfoBufferBarrier }, {});
			}
		}
	}

	void OctreeBuilder::cmdMipmapWrite(VkCommandBuffer cmdBufferHandle)
	{
		CommandBuffer cmdBuffer(cmdBufferHandle); // Variable hiding
		
		const uint32_t numVoxelFragment = _voxelizer->getFragmentCount();
		const uint32_t fragmentGroupX = numVoxelFragment / 64 + ((numVoxelFragment & 0x3f) > 0 ? 1 : 0);
		const uint32_t pushValues[] = { numVoxelFragment, _voxelizer->getVoxelResolution() };
		cmdBuffer.pushConstants(_pipelineLayout->getLayoutHandle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushValues), pushValues);
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, _pipelineLayout->getLayoutHandle(), 0, { _descriptorSet }, {});

		cmdBuffer.bindPipeline(_nodeLeafWritePipeline);
		cmdBuffer.dispatch(fragmentGroupX, 1, 1);

		VkBufferMemoryBarrier octreeBufferBarrier = _octreeBuffer->generateMemoryBarrier(
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
		);
		cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0, {}, { octreeBufferBarrier }, {});

		for (uint32_t i = 2; i <= _level; ++i)
		{
			uint32_t levelPush[] = { i };
			cmdBuffer.pushConstants(_pipelineLayout->getLayoutHandle(), VK_SHADER_STAGE_COMPUTE_BIT, sizeof(pushValues), sizeof(levelPush), levelPush);
			cmdBuffer.bindPipeline(_nodeMipmapWritePipeline);
			cmdBuffer.dispatch(fragmentGroupX, 1, 1);

			if (i != _level)
			{
				octreeBufferBarrier = _octreeBuffer->generateMemoryBarrier(
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
				);
				cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0, {}, { octreeBufferBarrier }, {});
			}
		}
	}

	void OctreeBuilder::transferOwnership(VkCommandBuffer cmdBuffer, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
		uint32_t srcQueueFamily, uint32_t dstQueueFamily)
	{
		const VkBufferMemoryBarrier octreeBufferBarrier = _octreeBuffer->generateMemoryBarrier(
			VK_ACCESS_NONE_KHR,
			VK_ACCESS_NONE_KHR,
			srcQueueFamily,
			dstQueueFamily
		);
		vkCmdPipelineBarrier(cmdBuffer, srcStage, dstStage, 0, 0, nullptr, 1, &octreeBufferBarrier, 0, nullptr);
	}

	void OctreeBuilder::readOctreeNodes(const CommandPoolPtr& cmdPool, OUT std::vector<glm::uvec2>* readData)
	{
		BufferPtr stagingBuffer = std::make_shared<Buffer>(_device->getMemoryAllocator(), 
														   sizeof(glm::uvec2) * _numOctreeNodes,
														   VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
														   VMA_MEMORY_USAGE_CPU_ONLY);

		CommandBuffer cmdBuffer(cmdPool->allocateCommandBuffer());
		{
			cmdBuffer.beginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			VkBufferCopy bufferCopyInfo = {};
			bufferCopyInfo.size = sizeof(glm::uvec2) * _numOctreeNodes;
			bufferCopyInfo.srcOffset = 0;
			bufferCopyInfo.dstOffset = 0;
			cmdBuffer.copyBuffer(_octreeBuffer.get(), stagingBuffer, { bufferCopyInfo });

			cmdBuffer.endRecord();
		}

		Fence fence(_device, 1, 0);
		cmdPool->getQueue()->submitCmdBuffer({ cmdBuffer }, &fence);

		if (!fence.waitForAllFences(UINT64_MAX))
		{
			std::cerr << "[VoxelEngine] Failed to wait fence read counter value\n";
		}

		readData->resize(_numOctreeNodes);
		stagingBuffer->downloadData(readData->data(), sizeof(glm::uvec2) * _numOctreeNodes);
	}

	void OctreeBuilder::readOctreeBricks(const CommandPoolPtr& cmdPool, OUT std::vector<uint32_t>* readData)
	{
		BufferPtr stagingBuffer = std::make_shared<Buffer>(_device->getMemoryAllocator(), 
														   sizeof(uint32_t) * _numOctreeBricks,
														   VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
														   VMA_MEMORY_USAGE_CPU_ONLY);

		CommandBuffer cmdBuffer(cmdPool->allocateCommandBuffer());
		{
			cmdBuffer.beginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);


			VkBufferCopy bufferCopyInfo = {};
			bufferCopyInfo.size = sizeof(uint32_t) * _numOctreeBricks;
			bufferCopyInfo.srcOffset = 0;
			bufferCopyInfo.dstOffset = 0;
			cmdBuffer.copyBuffer(_octreeBrickImageBuffer.get(), stagingBuffer, { bufferCopyInfo });

			cmdBuffer.endRecord();
		}

		Fence fence(_device, 1, 0);
		cmdPool->getQueue()->submitCmdBuffer({ cmdBuffer }, &fence);

		if (!fence.waitForAllFences(UINT64_MAX))
		{
			std::cerr << "[VoxelEngine] Failed to wait fence read counter value\n";
		}
		readData->resize(_numOctreeBricks);
		stagingBuffer->downloadData(readData->data(), sizeof(uint32_t) * _numOctreeBricks);
	}
}