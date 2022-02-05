// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/DescriptorPool.h>
#include <VulkanFramework/Device.h>
#include <cassert>

namespace vfs
{
	DescriptorPool::DescriptorPool(DevicePtr device,
		const std::vector<VkDescriptorPoolSize>& poolSizes,
		const uint32_t maxNumSets,
		VkDescriptorPoolCreateFlags flags)
	{
		assert(initialize(device, poolSizes, maxNumSets, flags));
	}

	DescriptorPool::~DescriptorPool()
	{
		destroyDescriptorPool();
	}

	void DescriptorPool::destroyDescriptorPool(void)
	{
		if (_descriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(_device->getDeviceHandle(), _descriptorPool, nullptr);
			_descriptorPool = VK_NULL_HANDLE;
		}
		_device.reset();
	}

	bool DescriptorPool::initialize(DevicePtr device,
									const std::vector<VkDescriptorPoolSize>& poolSizes,
									const uint32_t maxNumSets,
									VkDescriptorPoolCreateFlags flags)
	{
		_device = device;

		VkDescriptorPoolCreateInfo descPoolInfo = {};
		descPoolInfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descPoolInfo.pNext			= nullptr;
		descPoolInfo.pPoolSizes		= poolSizes.data();
		descPoolInfo.poolSizeCount	= static_cast<uint32_t>(poolSizes.size());
		descPoolInfo.maxSets		= maxNumSets;
		descPoolInfo.flags			= flags;

		if (vkCreateDescriptorPool(_device->getDeviceHandle(), &descPoolInfo, nullptr, &_descriptorPool) != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}
}