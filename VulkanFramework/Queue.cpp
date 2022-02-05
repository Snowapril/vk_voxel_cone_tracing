// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/CommandBuffer.h>
#include <VulkanFramework/Fence.h>

namespace vfs
{
	Queue::Queue(DevicePtr device,
				 uint32_t familyIndex)
	{
		assert(initialize(device, familyIndex));
	}
	Queue::~Queue()
	{
		destroyQueue();
	}

	void Queue::destroyQueue(void)
	{
		_device.reset();
		_familyIndex = 0;
	}

	bool Queue::initialize(DevicePtr device,
						   uint32_t familyIndex)
	{
		_device = device;
		_familyIndex = familyIndex;
		
		vkGetDeviceQueue(_device->getDeviceHandle(), _familyIndex, 0, &_queueHandle);
		return true;
	}

	void Queue::submitCmdBuffer(const std::vector<CommandBuffer>& cmdBuffers,
								const Fence* fence)
	{
		std::vector<VkCommandBuffer> cmdBufferHandles(cmdBuffers.size());
		for (size_t i = 0; i < cmdBuffers.size(); ++i)
		{
			cmdBufferHandles[i] = cmdBuffers[i].getHandle();
		}

		VkSubmitInfo submitInfo = {};
		submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pCommandBuffers		= cmdBufferHandles.data();
		submitInfo.commandBufferCount	= static_cast<uint32_t>(cmdBufferHandles.size());

		vkQueueSubmit(_queueHandle, 1, &submitInfo, fence == nullptr ? VK_NULL_HANDLE : fence->getFence(0));
	}

	
	void Queue::submitCmdBufferSynchronized(const std::vector<CommandBuffer>& cmdBuffers,
											const std::vector<VkSemaphore>& waitSemaphores,
											const std::vector<VkPipelineStageFlags>& waitDstStageMasks,
											const std::vector<VkSemaphore>& signalSemaphores,
											const Fence* fence)
	{
		std::vector<VkCommandBuffer> cmdBufferHandles(cmdBuffers.size());
		for (size_t i = 0; i < cmdBuffers.size(); ++i)
		{
			cmdBufferHandles[i] = cmdBuffers[i].getHandle();
		}

		VkSubmitInfo submitInfo = {};
		submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pCommandBuffers		= cmdBufferHandles.data();
		submitInfo.commandBufferCount	= static_cast<uint32_t>(cmdBufferHandles.size());
		submitInfo.waitSemaphoreCount	= static_cast<uint32_t>(waitSemaphores.size());
		submitInfo.pWaitSemaphores		= waitSemaphores.data();
		submitInfo.pWaitDstStageMask	= waitDstStageMasks.data();
		submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
		submitInfo.pSignalSemaphores	= signalSemaphores.data();
		vkQueueSubmit(_queueHandle, 1, &submitInfo, fence == nullptr ? VK_NULL_HANDLE : fence->getFence(0));
	}
}