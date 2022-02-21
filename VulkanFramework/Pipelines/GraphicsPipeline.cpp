// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Pipelines/GraphicsPipeline.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Pipelines/PipelineConfig.h>

namespace vfs
{
	GraphicsPipeline::GraphicsPipeline(std::shared_ptr<Device> device)
		: PipelineBase(device)
	{
		// Do nothing
	}

	bool GraphicsPipeline::initializePipeline(const PipelineConfig* pipelineConfig,
											  const std::vector<VkPipelineShaderStageCreateInfo>& shaderStageInfos)
	{
		assert(pipelineConfig != nullptr && pipelineConfig->isValid()); // snowapril : given pipeline configuration must be valid

		VkGraphicsPipelineCreateInfo graphicsPipelineInfo = {};
		graphicsPipelineInfo.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsPipelineInfo.pNext					= nullptr;
		graphicsPipelineInfo.basePipelineHandle		= VK_NULL_HANDLE;
		graphicsPipelineInfo.basePipelineIndex		= -1;
		graphicsPipelineInfo.layout					= pipelineConfig->pipelineLayout;
		graphicsPipelineInfo.pColorBlendState		= &pipelineConfig->colorBlendInfo;
		graphicsPipelineInfo.pDepthStencilState		= &pipelineConfig->depthStencilInfo;
		graphicsPipelineInfo.pDynamicState			= &pipelineConfig->dynamicStateInfo;
		graphicsPipelineInfo.pInputAssemblyState	= &pipelineConfig->inputAseemblyInfo;
		graphicsPipelineInfo.pMultisampleState		= &pipelineConfig->multisampleInfo;
		graphicsPipelineInfo.pRasterizationState	= &pipelineConfig->rasterizationInfo;
		graphicsPipelineInfo.stageCount				= static_cast<uint32_t>(shaderStageInfos.size());
		graphicsPipelineInfo.pStages				= shaderStageInfos.data();
		graphicsPipelineInfo.pVertexInputState		= &pipelineConfig->vertexInputInfo;
		graphicsPipelineInfo.pViewportState			= &pipelineConfig->viewportInfo;
		graphicsPipelineInfo.renderPass				= pipelineConfig->renderPass;
		graphicsPipelineInfo.subpass				= pipelineConfig->subPass;

		if (vkCreateGraphicsPipelines(_device->getDeviceHandle(), VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &_pipeline) != VK_SUCCESS)
		{
			return false;
		}

		return true;
	}
}