// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_QUEUE_H)
#define VULKAN_FRAMEWORK_QUEUE_H

#include <VulkanFramework/pch.h>
#include <VulkanFramework/CommandBuffer.h>

namespace vfs
{
	class Queue : NonCopyable
	{
	public:
		explicit Queue() = default;
		explicit Queue(DevicePtr device, 
					   uint32_t familyIndex);
				~Queue();

	public:
		bool			initialize					(DevicePtr device, uint32_t familyIndex);
		void			submitCmdBuffer				(const std::vector<CommandBuffer>& cmdBuffers, 
													 const Fence* fence);
		void			submitCmdBufferSynchronized	(const std::vector<CommandBuffer>& cmdBuffers,
													 const std::vector<VkSemaphore>& waitSemaphores,
													 const std::vector<VkPipelineStageFlags>& waitDstStageMasks,
													 const std::vector<VkSemaphore>& signalSemaphores,
													 const Fence* fence);

		inline VkQueue getQueueHandle(void) const
		{
			return _queueHandle;
		}
		inline uint32_t getFamilyIndex(void) const
		{
			return _familyIndex;
		}
		inline DevicePtr getDevicePtr(void) const
		{
			return _device;
		}

	private:
		DevicePtr	_device		 { nullptr };
		VkQueue		_queueHandle { VK_NULL_HANDLE };
		uint32_t	_familyIndex { 0 };
	};
}

#endif