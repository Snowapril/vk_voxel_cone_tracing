// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Fence.h>

namespace vfs
{
	Fence::Fence(DevicePtr device, uint32_t numFences, VkFenceCreateFlags flags)
	{
		assert(initialize(device, numFences, flags));
	}

	Fence::~Fence()
	{
		destroyFence();
	}

	void Fence::destroyFence(void)
	{
		for (VkFence& fence : _fences)
		{
			vkDestroyFence(_device->getDeviceHandle(), fence, nullptr);
		}
		_device.reset();
	}

	bool Fence::initialize(DevicePtr device, uint32_t numFences, VkFenceCreateFlags flags)
	{
		_device = device;
		_fences.resize(numFences);

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.pNext = nullptr;
		fenceInfo.flags = flags;
		for (size_t i = 0; i < _fences.size(); ++i)
		{
			if (vkCreateFence(_device->getDeviceHandle(), &fenceInfo, nullptr, &_fences[i]) != VK_SUCCESS)
			{
				for (size_t j = 0; j < i; ++j)
				{
					vkDestroyFence(_device->getDeviceHandle(), _fences[j], nullptr);
				}
				_fences.clear();
				return false;
			}
		}

		return true;
	}

	bool Fence::waitForFence(uint32_t fenceIndex, uint64_t timeout)
	{
		return vkWaitForFences(_device->getDeviceHandle(), 1, &_fences[fenceIndex], VK_TRUE, timeout) == VK_SUCCESS;
	}

	bool Fence::waitForAllFences(uint64_t timeout)
	{
		return vkWaitForFences(_device->getDeviceHandle(), static_cast<uint32_t>(_fences.size()), _fences.data(), VK_TRUE, timeout) == VK_SUCCESS;
	}

	bool Fence::waitForAnyFences(uint64_t timeout)
	{
		return vkWaitForFences(_device->getDeviceHandle(), static_cast<uint32_t>(_fences.size()), _fences.data(), VK_FALSE, timeout) == VK_SUCCESS;
	}

	bool Fence::resetFence(void)
	{
		return vkResetFences(_device->getDeviceHandle(), static_cast<uint32_t>(_fences.size()), _fences.data()) == VK_SUCCESS;
	}
}