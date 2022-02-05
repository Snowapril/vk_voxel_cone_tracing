// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_GRAPHICS_PIPELINE_H)
#define VULKAN_FRAMEWORK_GRAPHICS_PIPELINE_H

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Pipelines/PipelineBase.h>

namespace vfs
{
	class GraphicsPipeline : public PipelineBase
	{
	public:
		explicit GraphicsPipeline() = default;
		explicit GraphicsPipeline(std::shared_ptr<Device> device);
				~GraphicsPipeline() = default;

	public:
		VkPipelineBindPoint getBindPoint(void) const override
		{
			return VK_PIPELINE_BIND_POINT_GRAPHICS;
		}

	private:
		bool initializePipeline(const PipelineConfig* pipelineConfig,
								const std::vector<VkPipelineShaderStageCreateInfo>& shaderStageInfos) override;
	};
}

#endif