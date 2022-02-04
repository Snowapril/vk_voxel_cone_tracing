// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/ComputePipeline.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/PipelineConfig.h>
#include <iostream>

namespace vfs
{
	ComputePipeline::ComputePipeline(std::shared_ptr<Device> device)
		: PipelineBase(device)
	{
		// Do nothing
	}

	bool ComputePipeline::initializePipeline(const PipelineConfig* pipelineConfig, 
											 const std::vector<VkPipelineShaderStageCreateInfo>& shaderStageInfos)
	{
		assert(pipelineConfig != nullptr);

		VkComputePipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext = nullptr;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.stage  = shaderStageInfos[0];
		pipelineInfo.layout = pipelineConfig->pipelineLayout;

		if (vkCreateComputePipelines(_device->getDeviceHandle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS)
		{
			std::cerr << "[RenderEngine] Failed to create compute pipeline\n";
			return false;
		}

		return true;
	}
}