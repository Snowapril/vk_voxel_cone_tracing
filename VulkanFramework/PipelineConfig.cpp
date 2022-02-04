// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/PipelineConfig.h>

namespace vfs
{
	void PipelineConfig::GetDefaultConfig(OUT PipelineConfig* config)
	{
		config->vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		config->vertexInputInfo.pNext = nullptr;
		config->vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(config->attributeDesc.size());
		config->vertexInputInfo.pVertexAttributeDescriptions	= config->attributeDesc.data();
		config->vertexInputInfo.vertexBindingDescriptionCount	= static_cast<uint32_t>(config->bindingDesc.size());
		config->vertexInputInfo.pVertexBindingDescriptions		= config->bindingDesc.data();

		config->inputAseemblyInfo.sType		= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		config->inputAseemblyInfo.pNext		= nullptr;
		config->inputAseemblyInfo.topology	= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		config->inputAseemblyInfo.primitiveRestartEnable = VK_FALSE;

		config->viewportInfo.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		config->viewportInfo.pNext			= nullptr;
		config->viewportInfo.pScissors		= nullptr;
		config->viewportInfo.scissorCount	= 1;
		config->viewportInfo.pViewports		= nullptr;
		config->viewportInfo.viewportCount	= 1;

		config->rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		config->rasterizationInfo.pNext						= nullptr;
		config->rasterizationInfo.depthClampEnable			= VK_FALSE;
		config->rasterizationInfo.rasterizerDiscardEnable	= VK_FALSE;
		config->rasterizationInfo.polygonMode				= VK_POLYGON_MODE_FILL;
		config->rasterizationInfo.lineWidth					= 1.0f;
		config->rasterizationInfo.cullMode					= VK_CULL_MODE_NONE;
		config->rasterizationInfo.frontFace					= VK_FRONT_FACE_CLOCKWISE;
		config->rasterizationInfo.depthBiasEnable			= VK_FALSE;
		config->rasterizationInfo.depthBiasClamp			= 0.0f;
		config->rasterizationInfo.depthBiasConstantFactor	= 0.0f;
		config->rasterizationInfo.depthBiasSlopeFactor		= 0.0f;

		config->multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		config->multisampleInfo.pNext					= nullptr;
		config->multisampleInfo.sampleShadingEnable		= VK_FALSE;
		config->multisampleInfo.rasterizationSamples	= VK_SAMPLE_COUNT_1_BIT;
		config->multisampleInfo.minSampleShading		= 1.0f;
		config->multisampleInfo.pSampleMask				= nullptr;
		config->multisampleInfo.alphaToCoverageEnable	= VK_FALSE;
		config->multisampleInfo.alphaToOneEnable		= VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		colorBlendAttachment.colorWriteMask			= VK_COLOR_COMPONENT_R_BIT |
													  VK_COLOR_COMPONENT_G_BIT |
													  VK_COLOR_COMPONENT_B_BIT |
													  VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable			= VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor	= VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor	= VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp			= VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor	= VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor	= VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp			= VK_BLEND_OP_ADD;
		config->colorBlendAttachments.emplace_back(std::move(colorBlendAttachment));

		config->colorBlendInfo.sType			 = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		config->colorBlendInfo.pNext			 = nullptr;
		config->colorBlendInfo.attachmentCount	 = static_cast<uint32_t>(config->colorBlendAttachments.size());
		config->colorBlendInfo.pAttachments		 = config->colorBlendAttachments.data();
		config->colorBlendInfo.logicOpEnable	 = VK_FALSE;
		config->colorBlendInfo.logicOp			 = VK_LOGIC_OP_COPY;
		config->colorBlendInfo.blendConstants[0] = 0.0f;
		config->colorBlendInfo.blendConstants[1] = 0.0f;
		config->colorBlendInfo.blendConstants[2] = 0.0f;
		config->colorBlendInfo.blendConstants[3] = 0.0f;

		config->depthStencilInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		config->depthStencilInfo.pNext					= nullptr;
		config->depthStencilInfo.stencilTestEnable		= VK_FALSE;
		config->depthStencilInfo.depthTestEnable		= VK_TRUE;
		config->depthStencilInfo.depthWriteEnable		= VK_TRUE;
		config->depthStencilInfo.minDepthBounds			= 0.0f;
		config->depthStencilInfo.maxDepthBounds			= 1.0f;
		config->depthStencilInfo.depthCompareOp			= VK_COMPARE_OP_LESS;
		config->depthStencilInfo.depthBoundsTestEnable	= VK_FALSE;
		config->depthStencilInfo.front					= {};
		config->depthStencilInfo.back					= {};

		
		config->dynamicStateInfo.sType				= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		config->dynamicStateInfo.pNext				= nullptr;
		config->dynamicStateInfo.dynamicStateCount	= static_cast<uint32_t>(config->dynamicStates.size());
		config->dynamicStateInfo.pDynamicStates		= config->dynamicStates.data();
	}

	bool PipelineConfig::isValid(void) const
	{
		return	renderPass != VK_NULL_HANDLE		&& 
				pipelineLayout != VK_NULL_HANDLE;
	}
}