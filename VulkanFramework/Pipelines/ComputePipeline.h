// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_COMPUTE_PIPELINE_H)
#define VULKAN_FRAMEWORK_COMPUTE_PIPELINE_H

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Pipelines/PipelineBase.h>

namespace vfs
{
	class ComputePipeline : public PipelineBase
	{
	public:
		explicit ComputePipeline() = default;
		explicit ComputePipeline(std::shared_ptr<Device> device);
				~ComputePipeline() = default;

	public:
		VkPipelineBindPoint getBindPoint(void) const override
		{
			return VK_PIPELINE_BIND_POINT_COMPUTE;
		}

	private:
		bool initializePipeline(const PipelineConfig* pipelineConfig,
								const std::vector<VkPipelineShaderStageCreateInfo>& shaderStageInfos) override;
	};
}

#endif