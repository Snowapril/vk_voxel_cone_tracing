// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_PIPELINE_BASE_H)
#define VULKAN_FRAMEWORK_PIPELINE_BASE_H

#include <VulkanFramework/pch.h>
#include <memory>
#include <unordered_map>

namespace vfs
{
	class Device;
	struct PipelineConfig;

	class PipelineBase : NonCopyable
	{
	public:
		explicit PipelineBase() = default;
		explicit PipelineBase(std::shared_ptr<Device> device);
		virtual ~PipelineBase();

	public:
		void destroyPipeline(void);
		bool initialize		(std::shared_ptr<Device> device);
		bool createPipeline	(const PipelineConfig* pipelineConfig);
		void bindPipeline	(VkCommandBuffer commandBuffer);
		void addShaderModule(VkShaderStageFlagBits stage, const char* shaderPath, 
							 const VkSpecializationInfo* specialInfo);
		
		virtual VkPipelineBindPoint getBindPoint(void) const = 0;

		inline VkPipeline getHandle(void) const
		{
			return _pipeline;
		}

	private:
		virtual bool initializePipeline	(const PipelineConfig* pipelineConfig,
										 const std::vector<VkPipelineShaderStageCreateInfo>& shaderStageInfos) = 0;
		
		static VkShaderModule	CreateShaderModule	(VkDevice device, const std::vector<char>& shaderData);
		static bool				ReadSpirvShaderFile	(const char* filePath, std::vector<char>* data);

	protected:
		VkPipeline				_pipeline	{ VK_NULL_HANDLE };
		std::shared_ptr<Device> _device		{	nullptr		 };
		std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
	};
}

#endif