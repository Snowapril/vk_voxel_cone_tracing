// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Buffer.h>
#include <VulkanFramework/BufferView.h>
#include <VulkanFramework/DescriptorSet.h>
#include <VulkanFramework/DescriptorPool.h>
#include <VulkanFramework/DescriptorSetLayout.h>
#include <VulkanFramework/ImageView.h>
#include <VulkanFramework/Sampler.h>
#include <VulkanFramework/Device.h>

namespace vfs
{
	DescriptorSet::DescriptorSet(DevicePtr device,
								 DescriptorPoolPtr descriptorPool,
								 DescriptorSetLayoutPtr descriptorSetLayout,
								 const uint32_t descSetCount)
	{
		assert(initialize(device, 
						  descriptorPool, 
						  descriptorSetLayout, 
						  descSetCount));
	}

	DescriptorSet::~DescriptorSet()
	{
		destroyDescriptorSet();
	}

	void DescriptorSet::destroyDescriptorSet(void)
	{
		_descriptorSetLayout.reset();
		_descriptorPool.reset();
		_device.reset();
	}

	bool DescriptorSet::initialize(DevicePtr device,
								   DescriptorPoolPtr descriptorPool,
								   DescriptorSetLayoutPtr descriptorSetLayout,
								   const uint32_t descSetCount)
	{
		_device = device;
		_descriptorPool = descriptorPool;
		_descriptorSetLayout = descriptorSetLayout;

		const VkDescriptorSetLayout setLayout = _descriptorSetLayout->getLayoutHandle();
		VkDescriptorSetAllocateInfo descSetAllocInfo = {};
		descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAllocInfo.pNext = nullptr;
		descSetAllocInfo.descriptorPool = _descriptorPool->getPoolHandle();
		descSetAllocInfo.descriptorSetCount = descSetCount;
		descSetAllocInfo.pSetLayouts = &setLayout;

		if (vkAllocateDescriptorSets(_device->getDeviceHandle(), &descSetAllocInfo, &_descriptorSet) != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}
	
	void DescriptorSet::updateBuffer(const std::vector<BufferPtr>& buffers,
									 const uint32_t dstBinding, 
									 const uint32_t descCount,
									 VkDescriptorType descType)
	{
		std::vector<VkDescriptorBufferInfo> bufferInfos;
		bufferInfos.reserve(buffers.size());
		for (size_t i = 0; i < buffers.size(); ++i)
		{
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = buffers[i]->getBufferHandle();
			bufferInfo.offset = 0;
			bufferInfo.range = buffers[i]->getTotalSize();
			bufferInfos.emplace_back(std::move(bufferInfo));
		}

		VkWriteDescriptorSet writeSet = {};
		writeSet.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSet.pNext				= nullptr;
		writeSet.dstBinding			= dstBinding;
		writeSet.dstSet				= _descriptorSet;
		writeSet.descriptorCount	= descCount;
		writeSet.descriptorType		= descType;
		writeSet.pBufferInfo		= bufferInfos.data();
		
		vkUpdateDescriptorSets(_device->getDeviceHandle(), 1, &writeSet, 0, nullptr);
	}

	void DescriptorSet::updateTexelBuffer(const BufferViewPtr& bufferView,
										  const uint32_t dstBinding, 
										  const uint32_t descCount,
										  VkDescriptorType descType)
	{
		const VkBufferView texelBufferView = bufferView->getBufferViewHandle();

		VkWriteDescriptorSet writeSet = {};
		writeSet.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSet.pNext				= nullptr;
		writeSet.dstBinding			= dstBinding;
		writeSet.dstSet				= _descriptorSet;
		writeSet.descriptorCount	= descCount;
		writeSet.descriptorType		= descType;
		writeSet.pTexelBufferView	= &texelBufferView;
		
		vkUpdateDescriptorSets(_device->getDeviceHandle(), 1, &writeSet, 0, nullptr);
	}

	void DescriptorSet::updateImage(const std::vector<VkDescriptorImageInfo>& imageInfos,
									const uint32_t dstBinding,
									VkDescriptorType descType)
	{
		VkWriteDescriptorSet writeSet = {};
		writeSet.sType			 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSet.pNext			 = nullptr;
		writeSet.dstBinding		 = dstBinding;
		writeSet.dstSet			 = _descriptorSet;
		writeSet.descriptorCount = static_cast<uint32_t>(imageInfos.size());
		writeSet.descriptorType  = descType;
		writeSet.pImageInfo		 = imageInfos.data();

		vkUpdateDescriptorSets(_device->getDeviceHandle(), 1, &writeSet, 0, nullptr);
	}
}