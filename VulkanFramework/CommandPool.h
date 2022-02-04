// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_COMMAND_POOL_H)
#define VULKAN_FRAMEWORK_COMMAND_POOL_H

#include <VulkanFramework/pch.h>

namespace vfs
{
	class CommandBuffer;
	class Device;
	class Queue;

	class CommandPool: NonCopyable
	{
	public:
		explicit CommandPool() = default;
		explicit CommandPool(DevicePtr device, QueuePtr queue, VkCommandPoolCreateFlags flags);
				~CommandPool();

	public:
		bool initialize			(DevicePtr device, QueuePtr queue, VkCommandPoolCreateFlags flags);
		void destroyCommandPool	(void);
		VkCommandBuffer				 allocateCommandBuffer			(void);
		std::vector<VkCommandBuffer> allocateMultipleCommandBuffer	(const uint32_t numAlloc);
		void						 freeCommandBuffers				(const std::vector<VkCommandBuffer>& cmdBuffers);

		inline VkCommandPool getPoolHandle(void) const
		{
			return _poolHandle;
		}
		inline QueuePtr getQueue(void) const
		{
			return _queue;
		}
	private:
		DevicePtr		_device		{ nullptr };
		QueuePtr		_queue		{ nullptr };
		VkCommandPool	_poolHandle	{ VK_NULL_HANDLE };
	};
}

#endif