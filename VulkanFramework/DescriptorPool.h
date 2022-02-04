// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_DESCRIPTOR_POOL_H)
#define VULKAN_FRAMEWORK_DESCRIPTOR_POOL_H

#include <VulkanFramework/pch.h>
#include <memory>

namespace vfs
{
	class Device;
	class DescriptorSetLayout;
	class DescriptorSet;

	class DescriptorPool : NonCopyable
	{
	public:
		explicit DescriptorPool() = default;
		explicit DescriptorPool(DevicePtr device, 
								const std::vector<VkDescriptorPoolSize>& poolSizes,
								const uint32_t maxNumSets,
								VkDescriptorPoolCreateFlags flags);
				~DescriptorPool();

	public:
		void destroyDescriptorPool	(void);
		bool initialize				(DevicePtr device, 
									 const std::vector<VkDescriptorPoolSize>& poolSizes,
									 const uint32_t maxNumSets,
									 VkDescriptorPoolCreateFlags flags);
		
		inline VkDescriptorPool getPoolHandle(void) const
		{
			return _descriptorPool;
		}
	private:
		DevicePtr _device		{ nullptr };
		VkDescriptorPool _descriptorPool	{ VK_NULL_HANDLE };
	};
}

#endif