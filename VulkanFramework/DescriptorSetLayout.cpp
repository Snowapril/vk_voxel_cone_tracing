// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/DescriptorSetLayout.h>

namespace vfs
{
	DescriptorSetLayout::DescriptorSetLayout(DevicePtr device)
	{
		assert(initialize(device));
	}

	DescriptorSetLayout::~DescriptorSetLayout()
	{
		destroyDescriptorSetLayout();
	}

	void DescriptorSetLayout::destroyDescriptorSetLayout(void)
	{
		if (_descSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(_device->getDeviceHandle(), _descSetLayout, nullptr);
			_descSetLayout = VK_NULL_HANDLE;
		}
		_descBindingInfos.clear();
		_device.reset();
	}

	bool DescriptorSetLayout::initialize(DevicePtr device)
	{
		_device = device;
		return true;
	}

	void DescriptorSetLayout::addBinding(VkShaderStageFlags stageFlags,
										 uint32_t bindingPoint,
										 uint32_t descCount,
										 VkDescriptorType descType,
										 VkDescriptorBindingFlags bindingFlags)
	{
		VkDescriptorSetLayoutBinding bindingInfo = {};
		bindingInfo.stageFlags			= stageFlags;
		bindingInfo.binding				= bindingPoint;
		bindingInfo.descriptorCount		= descCount;
		bindingInfo.descriptorType		= descType;
		bindingInfo.pImmutableSamplers	= nullptr;

		_descBindingInfos.emplace_back(std::move(bindingInfo), bindingFlags);
	}

	bool DescriptorSetLayout::createDescriptorSetLayout(VkDescriptorSetLayoutCreateFlags flags)
	{
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
		std::vector<VkDescriptorBindingFlags> bindingFlags;
		layoutBindings.reserve(_descBindingInfos.size());
		bindingFlags.reserve(_descBindingInfos.size());

		for (const auto& info : _descBindingInfos)
		{
			layoutBindings.emplace_back(info.first);
			bindingFlags.emplace_back(info.second);
		}
		_descBindingInfos.clear();
		_descBindingInfos.shrink_to_fit();

		VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo = {};
		flagsInfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		flagsInfo.pNext			= nullptr;
		flagsInfo.bindingCount	= static_cast<uint32_t>(bindingFlags.size());
		flagsInfo.pBindingFlags = bindingFlags.data();

		VkDescriptorSetLayoutCreateInfo descSetLayoutInfo = {};
		descSetLayoutInfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descSetLayoutInfo.bindingCount	= static_cast<uint32_t>(layoutBindings.size());
		descSetLayoutInfo.pBindings		= layoutBindings.data();
		descSetLayoutInfo.pNext			= &flagsInfo;
		descSetLayoutInfo.flags			= flags;

		if (vkCreateDescriptorSetLayout(_device->getDeviceHandle(), &descSetLayoutInfo, nullptr, &_descSetLayout) != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}
}