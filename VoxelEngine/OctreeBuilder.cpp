// Author : Jihong Shin (rubikpril)

#include <VoxelEngine/pch.h>
#include <VoxelEngine/OctreeBuilder.h>
#include <VoxelEngine/Voxelizer.h>
#include <VoxelEngine/Counter.h>
#include <VulkanFramework/DescriptorPool.h>
#include <VulkanFramework/Fence.h>
#include <VulkanFramework/DescriptorSet.h>
#include <VulkanFramework/DescriptorSetLayout.h>
#include <VulkanFramework/PipelineConfig.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Buffer.h>
#include <Common/Utils.h>
#include <iostream>

namespace vfs
{
	OctreeBuilder::OctreeBuilder(DevicePtr device, std::shared_ptr<SparseVoxelizer> voxelizer)
	{
		assert(initialize(device, voxelizer));
	}

	OctreeBuilder::~OctreeBuilder()
	{
		destroyOctreeBuilder();
	}

	void OctreeBuilder::destroyOctreeBuilder(void)
	{
		if (_pipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(_device->getDeviceHandle(), _pipelineLayout, nullptr);
		}
	}

	bool OctreeBuilder::initialize(DevicePtr device, std::shared_ptr<SparseVoxelizer> voxelizer)
	{
		_device		= device;
		_voxelizer	= voxelizer;
		_level		= _voxelizer->getLevel();

		if (!initializeBuffers())
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

	bool OctreeBuilder::initializeBuffers(void)
	{
		const VmaAllocator allocator = _device->getMemoryAllocator();
		
		_infoBuffer = std::make_shared<Buffer>(allocator, sizeof(uint32_t) * 2,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);
		_infoStagingBuffer = std::make_shared<Buffer>(allocator, sizeof(uint32_t) * 2,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY);

		// TODO(rubikpril) : this hard-coded data
		uint32_t infoData[] = { 0, 8 };
		_infoStagingBuffer->uploadData(infoData, sizeof(infoData));

		_indirectBuffer = std::make_shared<Buffer>(allocator, sizeof(uint32_t) * 3,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);
		_indirectStagingBuffer = std::make_shared<Buffer>(allocator, sizeof(uint32_t) * 3,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY);

		// TODO(rubikpril) : this hard-coded data
		uint32_t indirectData[] = { 1, 1, 1 };
		_indirectStagingBuffer->uploadData(indirectData, sizeof(indirectData));

		_numOctreeNodes = vfs::min(
			vfs::max(MIN_OCTREE_NODE_NUM, _voxelizer->getFragmentCount() << 2u),
			MAX_OCTREE_NODE_NUM
		);
		_octreeBuffer = std::make_shared<Buffer>(allocator, _numOctreeNodes * sizeof(uint32_t),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // TODO(rubikpril) : remove transfer bit
			VMA_MEMORY_USAGE_GPU_ONLY);
		
		_octreeNodeCounter = std::make_shared<Counter>();
		if (!_octreeNodeCounter->initialize(_device))
		{
			std::cerr << "[VoxelEngine] Failed to create counter\n";
			return false;
		}

		// TODO(rubikpril) : main command pool remove
		_octreeNodeCounter->resetCounter(_device->getCommandPool());

		std::clog << "[VoxelEngine] Octree node allocated : " << _numOctreeNodes / 1000 << " KB\n";
		return true;
	}

	bool OctreeBuilder::initializeDescriptors(void)
	{
		const std::vector<VkDescriptorPoolSize> poolSizes = {
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5},
		};
		_descriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 5, 0);  // TODO(rubikpril) : 5 is this good ?

		_descriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0);
		if (!_descriptorLayout->createDescriptorSetLayout())
		{
			return false;
		}

		_descriptorSet = std::make_unique<DescriptorSet>(_device, _descriptorPool, _descriptorLayout, 1);
		_descriptorSet->updateStorageBuffer(_octreeNodeCounter->getBuffer(),			0, 1);
		_descriptorSet->updateStorageBuffer(_octreeBuffer,					1, 1);
		_descriptorSet->updateStorageBuffer(_voxelizer->getFramgnetList(),	2, 1);
		_descriptorSet->updateStorageBuffer(_infoBuffer,					3, 1);
		_descriptorSet->updateStorageBuffer(_indirectBuffer,				4, 1);
		return true;
	}

	bool OctreeBuilder::initializePipelineLayout(void)
	{
		VkPushConstantRange pushConstRange = {};
		pushConstRange.offset = 0;
		pushConstRange.size = sizeof(uint32_t) * 2;
		pushConstRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		const VkDescriptorSetLayout descSetLayout = _descriptorLayout->getLayoutHandle();
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pNext = nullptr;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstRange;

		if (vkCreatePipelineLayout(_device->getDeviceHandle(), &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}

	bool OctreeBuilder::initializePipelines(void)
	{
		assert(_pipelineLayout != VK_NULL_HANDLE); // rubikpril : pipeline layout must be initialized first
		PipelineConfig config;
		config.pipelineLayout = _pipelineLayout;

		_nodeInitPipeline = std::make_unique<ComputePipeline>();
		_nodeInitPipeline->initialize(_device, { { VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/octreeNodeInit.comp.spv" } });
		_nodeInitPipeline->createPipeline(&config);
		
		_nodeFlagPipeline = std::make_unique<ComputePipeline>();
		_nodeFlagPipeline->initialize(_device, { { VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/octreeNodeFlag.comp.spv" } });
		_nodeFlagPipeline->createPipeline(&config);

		_nodeAllocPipeline = std::make_unique<ComputePipeline>();
		_nodeAllocPipeline->initialize(_device, { { VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/octreeNodeAlloc.comp.spv" } });
		_nodeAllocPipeline->createPipeline(&config);

		_nodeModifyArgPipeline = std::make_unique<ComputePipeline>();
		_nodeModifyArgPipeline->initialize(_device, { { VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/octreeNodeModifyArg.comp.spv" } });
		_nodeModifyArgPipeline->createPipeline(&config);

		return true;
	}

	void OctreeBuilder::cmdBuild(VkCommandBuffer cmdBuffer)
	{
		{
			VkBufferCopy copyRegion = {};
			copyRegion.size = sizeof(uint32_t) * 2;
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			vkCmdCopyBuffer(cmdBuffer, _infoStagingBuffer->getBufferHandle(), _infoBuffer->getBufferHandle(),
				1, &copyRegion);

			copyRegion.size = sizeof(uint32_t) * 3;
			vkCmdCopyBuffer(cmdBuffer, _indirectStagingBuffer->getBufferHandle(), _indirectBuffer->getBufferHandle(),
				1, &copyRegion);

			VkBufferMemoryBarrier infoBufferBarrier = _infoBuffer->generateMemoryBarrier(
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
			);
			vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0, 0, nullptr,
				1, &infoBufferBarrier, 0, nullptr);
			VkBufferMemoryBarrier indirectBufferBarrier = _indirectBuffer->generateMemoryBarrier(
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
			);
			vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0, 0, nullptr,
				1, &indirectBufferBarrier, 0, nullptr);
		}
		
		const uint32_t numVoxelFragment = _voxelizer->getFragmentCount();
		const uint32_t fragmentGroupX = numVoxelFragment / 64 + ((numVoxelFragment & 0x3f) > 0 ? 1 : 0);
		uint32_t pushValues[] = { numVoxelFragment, _voxelizer->getVoxelResolution() };
		vkCmdPushConstants(cmdBuffer, _pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushValues), pushValues);
		const VkDescriptorSet descSet = _descriptorSet->getHandle();
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipelineLayout, 0, 1, &descSet, 0, nullptr);

		for (uint32_t i = 1; i <= _level; ++i)
		{
			vkCmdBindPipeline(cmdBuffer, _nodeInitPipeline->getBindPoint(), _nodeInitPipeline->getHandle());
			vkCmdDispatchIndirect(cmdBuffer, _indirectBuffer->getBufferHandle(), 0);
			
			VkBufferMemoryBarrier octreeBufferBarrier = _octreeBuffer->generateMemoryBarrier(
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
			);
			vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0, 0, nullptr,
				1, &octreeBufferBarrier, 0, nullptr);

			vkCmdBindPipeline(cmdBuffer, _nodeFlagPipeline->getBindPoint(), _nodeFlagPipeline->getHandle());
			vkCmdDispatch(cmdBuffer, fragmentGroupX, 1, 1);

			if (i != _level)
			{
				octreeBufferBarrier = _octreeBuffer->generateMemoryBarrier(
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
				);
				vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0, 0, nullptr,
					1, &octreeBufferBarrier, 0, nullptr);
				
				vkCmdBindPipeline(cmdBuffer, _nodeAllocPipeline->getBindPoint(), _nodeAllocPipeline->getHandle());
				vkCmdDispatchIndirect(cmdBuffer, _indirectBuffer->getBufferHandle(), 0); // TODO(rubikpril) : offset

				octreeBufferBarrier = _octreeBuffer->generateMemoryBarrier(
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
				);
				vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0, 0, nullptr,
					1, &octreeBufferBarrier, 0, nullptr);

				vkCmdBindPipeline(cmdBuffer, _nodeModifyArgPipeline->getBindPoint(), _nodeModifyArgPipeline->getHandle());
				vkCmdDispatch(cmdBuffer, 1, 1, 1);

				VkBufferMemoryBarrier indirectBufferBarrier = _indirectBuffer->generateMemoryBarrier(
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
				);
				vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0, 0, nullptr,
					1, &indirectBufferBarrier, 0, nullptr);

				VkBufferMemoryBarrier buildInfoBufferBarrier = _infoBuffer->generateMemoryBarrier(
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT
				);
				vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0, 0, nullptr,
					1, &buildInfoBufferBarrier, 0, nullptr);
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

	void OctreeBuilder::readOctreeNodes(VkCommandPool commandPool, OUT std::vector<uint32_t>* readData)
	{
		Buffer stagingBuffer;
		stagingBuffer.initialize(_device->getMemoryAllocator(), sizeof(uint32_t) * _numOctreeNodes,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		VkCommandBufferAllocateInfo cmdBufferAllocateInfo = {};
		cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocateInfo.pNext = nullptr;
		cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufferAllocateInfo.commandPool = commandPool;
		cmdBufferAllocateInfo.commandBufferCount = 1;

		VkCommandBuffer cmdBuffer{ VK_NULL_HANDLE };
		if (vkAllocateCommandBuffers(_device->getDeviceHandle(), &cmdBufferAllocateInfo, &cmdBuffer) != VK_SUCCESS)
		{
			std::cerr << "[VoxelEngine] Failed to allocate command buffer for read counter value\n";
		}

		VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBeginInfo.pNext = nullptr;
		cmdBufferBeginInfo.pInheritanceInfo = nullptr;
		cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo) != VK_SUCCESS)
		{
			std::cerr << "[VoxelEngine] Failed to begin command buffer for read counter value\n";
		}

		VkBufferCopy bufferCopyInfo = {};
		bufferCopyInfo.size = sizeof(uint32_t) * _numOctreeNodes;
		bufferCopyInfo.srcOffset = 0;
		bufferCopyInfo.dstOffset = 0;
		vkCmdCopyBuffer(cmdBuffer, _octreeBuffer->getBufferHandle(), stagingBuffer.getBufferHandle(), 1, &bufferCopyInfo);

		if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS)
		{
			std::cerr << "[VoxelEngine] Failed to end command buffer for read counter value\n";
		}

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;

		Fence fence(_device, 1, 0);
		if (vkQueueSubmit(_device->getGraphicsQueue(), 1, &submitInfo, fence.getFence(0)) != VK_SUCCESS)
		{
			std::cerr << "[VoxelEngine] Failed to submit command buffer to "
				"graphics queue for read counter value\n";
		}

		if (!fence.waitForAllFences(UINT64_MAX))
		{
			std::cerr << "[VoxelEngine] Failed to wait fence read counter value\n";
		}

		vkFreeCommandBuffers(_device->getDeviceHandle(), commandPool, 1, &cmdBuffer);

		readData->resize(_numOctreeNodes);
		stagingBuffer.downloadData(readData->data(), sizeof(uint32_t) * _numOctreeNodes);

		// for testing
		uint32_t data = _octreeNodeCounter->readCounterValue(commandPool);
	}
}