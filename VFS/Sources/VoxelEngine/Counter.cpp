// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <VulkanFramework/Buffers/Buffer.h>
#include <VulkanFramework/Commands/CommandPool.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Sync/Fence.h>
#include <VoxelEngine/Counter.h>
#include <iostream>

namespace vfs
{
	Counter::Counter(DevicePtr device)
	{
		assert(initialize(device));
	}

	Counter::~Counter()
	{
		destroyCounter();
	}

	void Counter::destroyCounter(void)
	{
		_stagingBuffer.reset();
		_buffer.reset();
	}

	bool Counter::initialize(DevicePtr device)
	{
		_device			= device;
		_buffer			= std::make_shared<Buffer>();
		_stagingBuffer	= std::make_shared<Buffer>();

		if (!_buffer->initialize(_device->getMemoryAllocator(), sizeof(uint32_t), 
								 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
								 VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
								 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
								 VMA_MEMORY_USAGE_GPU_ONLY))
		{
			return false;
		}

		if (!_stagingBuffer->initialize(_device->getMemoryAllocator(), sizeof(uint32_t), 
										VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
										VK_BUFFER_USAGE_TRANSFER_DST_BIT,
										VMA_MEMORY_USAGE_CPU_ONLY))
		{
			return false;
		}

		return true;
	}

	uint32_t Counter::readCounterValue(const CommandPoolPtr& cmdPool)
	{
		CommandBuffer cmdBuffer(cmdPool->allocateCommandBuffer());
		{
			cmdBuffer.beginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			VkBufferCopy bufferCopyInfo = {};
			bufferCopyInfo.size = sizeof(uint32_t);
			bufferCopyInfo.srcOffset = 0;
			bufferCopyInfo.dstOffset = 0;
			cmdBuffer.copyBuffer(_buffer.get(), _stagingBuffer, { bufferCopyInfo });

			cmdBuffer.endRecord();
		}
		
		Fence fence(_device, 1, 0);
		cmdPool->getQueue()->submitCmdBuffer({ cmdBuffer }, &fence);

		if (!fence.waitForAllFences(UINT64_MAX))
		{
			std::cerr << "[VoxelEngine] Failed to wait fence read counter value\n";
			return 0;
		}

		uint32_t readCounterValue{ 0 };
		_stagingBuffer->downloadData(&readCounterValue, sizeof(uint32_t));
		return readCounterValue;
	}

	void Counter::resetCounter(const CommandPoolPtr& cmdPool)
	{
		constexpr uint32_t RESET_VALUE = 0;
		_stagingBuffer->uploadData(&RESET_VALUE, sizeof(uint32_t));

		CommandBuffer cmdBuffer(cmdPool->allocateCommandBuffer());
		{
			cmdBuffer.beginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			VkBufferCopy bufferCopyInfo = {};
			bufferCopyInfo.size = sizeof(uint32_t);
			bufferCopyInfo.srcOffset = 0;
			bufferCopyInfo.dstOffset = 0;
			cmdBuffer.copyBuffer(_stagingBuffer.get(), _buffer, { bufferCopyInfo });

			cmdBuffer.endRecord();
		}

		Fence fence(_device, 1, 0);
		cmdPool->getQueue()->submitCmdBuffer({ cmdBuffer }, &fence);

		fence.waitForAllFences(UINT64_MAX);
	}
}