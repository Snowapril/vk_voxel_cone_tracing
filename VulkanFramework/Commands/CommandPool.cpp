// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Commands/CommandPool.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Queue.h>

namespace vfs
{
	CommandPool::CommandPool(DevicePtr device, QueuePtr queue, VkCommandPoolCreateFlags flags)
	{
		assert(initialize(device, queue, flags));
	}

	CommandPool::~CommandPool()
	{
		destroyCommandPool();
	}

	bool CommandPool::initialize(DevicePtr device, QueuePtr queue, VkCommandPoolCreateFlags flags)
	{
		_device = device;
		_queue = queue;

		VkCommandPoolCreateInfo commandPoolInfo = {};
		commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolInfo.pNext = nullptr;
		commandPoolInfo.queueFamilyIndex = _queue->getFamilyIndex();
		commandPoolInfo.flags = flags;

		if (vkCreateCommandPool(_device->getDeviceHandle(), &commandPoolInfo, nullptr, &_poolHandle) != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}

	void CommandPool::destroyCommandPool(void)
	{
		if (_poolHandle != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(_device->getDeviceHandle(), _poolHandle, nullptr);
			_poolHandle = VK_NULL_HANDLE;
		}
		_queue.reset();
		_device.reset();
	}

	VkCommandBuffer CommandPool::allocateCommandBuffer(void)
	{
		VkCommandBuffer cmdBuffer; 
		VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
		cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocInfo.pNext = nullptr;
		cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufferAllocInfo.commandBufferCount = 1;
		cmdBufferAllocInfo.commandPool = _poolHandle;
		vkAllocateCommandBuffers(_device->getDeviceHandle(), &cmdBufferAllocInfo, &cmdBuffer);
		return cmdBuffer;
	}

	std::vector<VkCommandBuffer> CommandPool::allocateMultipleCommandBuffer(const uint32_t numAlloc)
	{
		std::vector<VkCommandBuffer> cmdBuffers(numAlloc);
		VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
		cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocInfo.pNext = nullptr;
		cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufferAllocInfo.commandBufferCount = numAlloc;
		cmdBufferAllocInfo.commandPool = _poolHandle;
		vkAllocateCommandBuffers(_device->getDeviceHandle(), &cmdBufferAllocInfo, cmdBuffers.data());
		return cmdBuffers;
	}

	void CommandPool::freeCommandBuffers(const std::vector<VkCommandBuffer>& cmdBuffers)
	{
		vkFreeCommandBuffers(_device->getDeviceHandle(), _poolHandle, 
			static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data());
	}
}