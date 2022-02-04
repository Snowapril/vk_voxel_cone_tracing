// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_PIPELINE_CONFIG_H)
#define VULKAN_FRAMEWORK_PIPELINE_CONFIG_H

#include <VulkanFramework/pch.h>

namespace vfs
{
	struct PipelineConfig
	{	
	public:
		static	void GetDefaultConfig	(OUT PipelineConfig* config);
				bool isValid			(void) const;
	public:
		VkPipelineVertexInputStateCreateInfo			 vertexInputInfo		{};
		std::vector<VkVertexInputAttributeDescription>	 attributeDesc;
		std::vector<VkVertexInputBindingDescription>	 bindingDesc;
		VkPipelineInputAssemblyStateCreateInfo			 inputAseemblyInfo		{};
		VkPipelineViewportStateCreateInfo				 viewportInfo			{};
		VkPipelineRasterizationStateCreateInfo			 rasterizationInfo		{};
		VkPipelineMultisampleStateCreateInfo			 multisampleInfo		{};
		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
		VkPipelineColorBlendStateCreateInfo				 colorBlendInfo			{};
		VkPipelineDepthStencilStateCreateInfo			 depthStencilInfo		{};
		std::vector<VkDynamicState>						 dynamicStates;
		VkPipelineDynamicStateCreateInfo				 dynamicStateInfo		{};
		VkPipelineLayout								 pipelineLayout			{ VK_NULL_HANDLE };
		VkRenderPass									 renderPass				{ VK_NULL_HANDLE };
		uint32_t										 subPass				{ 0 };
	};
}

#endif