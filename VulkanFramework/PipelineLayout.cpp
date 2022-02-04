// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/PipelineLayout.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/DescriptorSetLayout.h>

namespace vfs
{
	PipelineLayout::PipelineLayout(DevicePtr device,
								   const std::vector<DescriptorSetLayoutPtr>& descSetLayouts,
								   const std::vector<VkPushConstantRange>& pushConstants)
	{
		assert(initialize(device, descSetLayouts, pushConstants));
	}

	PipelineLayout::~PipelineLayout()
	{
		destroyPipelineLayout();
	}

	bool PipelineLayout::initialize(DevicePtr device, 
									const std::vector<DescriptorSetLayoutPtr>& descSetLayouts,
									const std::vector<VkPushConstantRange>& pushConstants)
	{
		_device = device;

		std::vector<VkDescriptorSetLayout> layoutHandles(descSetLayouts.size());
		for (size_t i = 0; i < descSetLayouts.size(); ++i)
		{
			layoutHandles[i] = descSetLayouts[i]->getLayoutHandle();
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pNext					= nullptr;
		pipelineLayoutInfo.setLayoutCount			= static_cast<uint32_t>(layoutHandles.size());
		pipelineLayoutInfo.pSetLayouts				= layoutHandles.data();
		pipelineLayoutInfo.pushConstantRangeCount	= static_cast<uint32_t>(pushConstants.size());
		pipelineLayoutInfo.pPushConstantRanges		= pushConstants.data();
		return vkCreatePipelineLayout(_device->getDeviceHandle(), &pipelineLayoutInfo, nullptr, &_layoutHandle)
			== VK_SUCCESS;
	}

	void PipelineLayout::destroyPipelineLayout(void)
	{
		if (_layoutHandle != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(_device->getDeviceHandle(), _layoutHandle, nullptr);
		}
	}
};