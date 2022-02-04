// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_FRAME_BUFFER_H)
#define VULKAN_FRAMEWORK_FRAME_BUFFER_H

#include <VulkanFramework/pch.h>
#include <memory>

namespace vfs
{
	class Device;

	class Framebuffer : NonCopyable
	{
	public:
		explicit Framebuffer() = default;
		explicit Framebuffer(std::shared_ptr<Device> device, 
							 const std::vector<VkImageView>& attachments, 
							 VkRenderPass renderPass, 
							 VkExtent2D extent);
				~Framebuffer();

	public:
		void destroyFramebuffer(void);
		bool initialize(std::shared_ptr<Device> device, 
						const std::vector<VkImageView>& attachments, 
						VkRenderPass renderPass, 
						VkExtent2D extent);
		
		inline VkFramebuffer getFramebufferHandle(void) const
		{
			return _framebuffer;
		}
		inline VkExtent2D getExtent(void) const
		{
			return _extent;
		}
	
	private:
		std::shared_ptr<Device>	 _device		{	nullptr		 };
		VkFramebuffer			 _framebuffer	{ VK_NULL_HANDLE };
		VkExtent2D				 _extent		{		0,	0	 };
	};
}

#endif