// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Semaphore.h>

namespace vfs
{
	Semaphore::Semaphore(DevicePtr device)
	{
		assert(initialize(device));
	}
	
	Semaphore::~Semaphore()
	{
		destroySemaphore();
	}

	void Semaphore::destroySemaphore(void)
	{
		vkDestroySemaphore(_device->getDeviceHandle(), _semaphore, nullptr);
	}

	bool Semaphore::initialize(DevicePtr device)
	{
		_device = device;

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreInfo.pNext = nullptr;
		semaphoreInfo.flags = 0;

		if (vkCreateSemaphore(_device->getDeviceHandle(), &semaphoreInfo, nullptr, &_semaphore) != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}

}