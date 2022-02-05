// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_PIPELINE_LAYOUT)
#define VULKAN_FRAMEWORK_PIPELINE_LAYOUT

#include <VulkanFramework/pch.h>
#include <memory>

namespace vfs
{
	class Device;

	class PipelineLayout : NonCopyable
	{
	public:
		explicit PipelineLayout() = default;
		explicit PipelineLayout(DevicePtr device,
								const std::vector<DescriptorSetLayoutPtr>& descSetLayouts,
								const std::vector<VkPushConstantRange>& pushConstants);
		~PipelineLayout();

	public:
		bool initialize(DevicePtr device,
						const std::vector<DescriptorSetLayoutPtr>& descSetLayouts,
						const std::vector<VkPushConstantRange>& pushConstants);
		void destroyPipelineLayout(void);
		
		inline VkPipelineLayout getLayoutHandle(void) const
		{
			return _layoutHandle;
		}

	private:
		DevicePtr			_device			{ nullptr };
		VkPipelineLayout	_layoutHandle	{ VK_NULL_HANDLE };
	};
}

#endif