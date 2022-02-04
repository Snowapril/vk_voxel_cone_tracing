// Author : Jihong Shin (rubikpril)

#include <VoxelEngine/pch.h>
#include <VulkanFramework/Buffer.h>
#include <VulkanFramework/CommandPool.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Fence.h>
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
		_stagingBuffer	= std::make_unique<Buffer>();

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

	uint32_t Counter::readCounterValue(VkCommandPool cmdPool)
	{
		VkCommandBufferAllocateInfo cmdBufferAllocateInfo = {};
		cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocateInfo.pNext = nullptr;
		cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufferAllocateInfo.commandPool = cmdPool;
		cmdBufferAllocateInfo.commandBufferCount = 1;

		VkCommandBuffer cmdBuffer { VK_NULL_HANDLE };
		if (vkAllocateCommandBuffers(_device->getDeviceHandle(), &cmdBufferAllocateInfo, &cmdBuffer) != VK_SUCCESS)
		{
			std::cerr << "[VoxelEngine] Failed to allocate command buffer for read counter value\n";
			return 0;
		}

		VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBeginInfo.pNext = nullptr;
		cmdBufferBeginInfo.pInheritanceInfo = nullptr;
		cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo) != VK_SUCCESS)
		{
			std::cerr << "[VoxelEngine] Failed to begin command buffer for read counter value\n";
			return 0;
		}

		VkBufferCopy bufferCopyInfo = {};
		bufferCopyInfo.size = sizeof(uint32_t);
		bufferCopyInfo.srcOffset = 0;
		bufferCopyInfo.dstOffset = 0;
		vkCmdCopyBuffer(cmdBuffer, _buffer->getBufferHandle(), _stagingBuffer->getBufferHandle(), 1, &bufferCopyInfo);

		if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS)
		{
			std::cerr << "[VoxelEngine] Failed to end command buffer for read counter value\n";
			return 0;
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
			return 0;
		}

		if (!fence.waitForAllFences(UINT64_MAX))
		{
			std::cerr << "[VoxelEngine] Failed to wait fence read counter value\n";
			return 0;
		}

		vkFreeCommandBuffers(_device->getDeviceHandle(), cmdPool, 1, &cmdBuffer);

		uint32_t readCounterValue{ 0 };
		_stagingBuffer->downloadData(&readCounterValue, sizeof(uint32_t));
		return readCounterValue;
	}
	
	void Counter::resetCounter(VkCommandPool cmdPool)
	{
		constexpr uint32_t RESET_VALUE = 0;
		_stagingBuffer->uploadData(&RESET_VALUE, sizeof(uint32_t));

		VkCommandBufferAllocateInfo cmdBufferAllocateInfo = {};
		cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocateInfo.pNext = nullptr;
		cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufferAllocateInfo.commandPool = cmdPool;
		cmdBufferAllocateInfo.commandBufferCount = 1;

		VkCommandBuffer cmdBuffer{ VK_NULL_HANDLE };
		if (vkAllocateCommandBuffers(_device->getDeviceHandle(), &cmdBufferAllocateInfo, &cmdBuffer) != VK_SUCCESS)
		{
			std::cerr << "[VoxelEngine] Failed to allocate command buffer for reset counter value\n";
		}

		VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBeginInfo.pNext = nullptr;
		cmdBufferBeginInfo.pInheritanceInfo = nullptr;
		cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo) != VK_SUCCESS)
		{
			std::cerr << "[VoxelEngine] Failed to begin command buffer for reset counter value\n";
		}

		VkBufferCopy bufferCopyInfo = {};
		bufferCopyInfo.size = sizeof(uint32_t);
		bufferCopyInfo.srcOffset = 0;
		bufferCopyInfo.dstOffset = 0;
		vkCmdCopyBuffer(cmdBuffer, _stagingBuffer->getBufferHandle(), _buffer->getBufferHandle(), 1, &bufferCopyInfo);

		if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS)
		{
			std::cerr << "[VoxelEngine] Failed to end command buffer for reset counter value\n";
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
						 "graphics queue for reset counter value\n";
		}

		if (!fence.waitForAllFences(UINT64_MAX))
		{
			std::cerr << "[VoxelEngine] Failed to wait fence reset counter value\n";
		}

		vkFreeCommandBuffers(_device->getDeviceHandle(), cmdPool, 1, &cmdBuffer);
	}
}