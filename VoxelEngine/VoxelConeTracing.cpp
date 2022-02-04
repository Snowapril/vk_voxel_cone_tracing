// Author : Jihong Shin (rubikpril)

#include <VoxelEngine/pch.h>
#include <VoxelEngine/VoxelConeTracing.h>
#include <VulkanFramework/FrameLayout.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/PipelineConfig.h>
#include <iostream>

namespace vfs
{
	VoxelConeTracing::VoxelConeTracing(DevicePtr device, VkRenderPass renderPass)
	{
		assert(initialize(device, renderPass));
	}

	VoxelConeTracing::~VoxelConeTracing()
	{
		destroyVoxelConeTraicng();
	}

	void VoxelConeTracing::destroyVoxelConeTraicng(void)
	{
		vkDestroyPipelineLayout(_device->getDeviceHandle(), _pipelineLayout, nullptr);
		_pipeline.reset();
		_device.reset();
	}

	bool VoxelConeTracing::initialize(DevicePtr device, VkRenderPass renderPass)
	{
		_device = device;

		if (!initializePipelineLayout())
		{
			std::cerr << "[VulkanFramework] Failed to create pipeline layout\n";
			return false;
		}

		if (!initializePipeline(renderPass))
		{
			std::cerr << "[VulkanFramework] Failed to create pipeline\n";
			return false;
		}

		return true;
	}

	bool VoxelConeTracing::initializePipelineLayout(void)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pNext = nullptr;
		// TODO(rubikpril) : config pipeline layout info
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(_device->getDeviceHandle(), &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}

	bool VoxelConeTracing::initializePipeline(VkRenderPass renderPass)
	{
		assert(_pipelineLayout != VK_NULL_HANDLE); // rubikpril : pipeline layout must be initialized first

		PipelineConfig config;
		PipelineConfig::GetDefaultConfig(&config);

		// TODO(rubikpril) : Fix hard-coded viewport & scissor
		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		viewport.width = 1280;
		viewport.height = 920;
		config.viewportInfo.pViewports = &viewport;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent.width = 1280;
		scissor.extent.height = 920;
		config.viewportInfo.pScissors = &scissor;
		config.renderPass = renderPass;
		config.pipelineLayout = _pipelineLayout;

		std::vector<std::pair<VkShaderStageFlagBits, const char*>> shaderStages {
			{VK_SHADER_STAGE_VERTEX_BIT,	"Shaders/voxel_cone_tracing.vert.spv" },
			{VK_SHADER_STAGE_FRAGMENT_BIT,	"Shaders/voxel_cone_tracing.frag.spv" },
		};
		_pipeline = std::make_unique<GraphicsPipeline>(_device, std::move(shaderStages));
		_pipeline->createPipeline(&config);
		return true;
	}

	void VoxelConeTracing::render(const FrameLayout* frame)
	{
		_pipeline->bindPipeline(frame->commandBuffer);
		vkCmdDraw(frame->commandBuffer, 3, 1, 0, 0);
	}
}