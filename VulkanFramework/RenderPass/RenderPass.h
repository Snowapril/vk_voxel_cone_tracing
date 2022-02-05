// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_RENDER_PASS_H)
#define VULKAN_FRAMEWORK_RENDER_PASS_H

#include <VulkanFramework/pch.h>

namespace vfs
{
	class Device;

	class RenderPass : NonCopyable
	{
	public:
		explicit RenderPass() = default;
		explicit RenderPass(const DevicePtr&device, 
							const std::vector<VkAttachmentDescription>& attachmentDescs,
							const std::vector<VkSubpassDependency>& subpassDependencies,
							const bool isDepthIncluded);
				~RenderPass();

	public:
		bool initialize			(const DevicePtr& device,
								 const std::vector<VkAttachmentDescription>& attachmentDescs,
								 const std::vector<VkSubpassDependency>& subpassDependencies,
								 const bool isDepthIncluded);
		void destroyRenderPass	(void);

		inline VkRenderPass getHandle(void) const
		{
			return _renderPassHandle;
		}
	private:
		DevicePtr		_device{ nullptr };
		VkRenderPass	_renderPassHandle{ nullptr };
	};
}

#endif