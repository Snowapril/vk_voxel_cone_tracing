// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_DESCRIPTOR_SET_LAYOUT_H)
#define VULKAN_FRAMEWORK_DESCRIPTOR_SET_LAYOUT_H

#include <VulkanFramework/pch.h>
#include <memory>

namespace vfs
{
	class Device;

	class DescriptorSetLayout : NonCopyable
	{
	public:
		explicit DescriptorSetLayout() = default;
		explicit DescriptorSetLayout(DevicePtr device);
				~DescriptorSetLayout();

	public:
		bool initialize					(DevicePtr device);
		bool createDescriptorSetLayout	(VkDescriptorSetLayoutCreateFlags flags);
		void destroyDescriptorSetLayout	(void);
		void addBinding					(VkShaderStageFlags stageFlags,
							  			 uint32_t bindingPoint,
										 uint32_t descCount, 
										 VkDescriptorType descType, 
										 VkDescriptorBindingFlags bindingFlags);
		inline VkDescriptorSetLayout getLayoutHandle(void) const
		{
			return _descSetLayout;
		}

	private:
		DevicePtr _device			{ nullptr };
		VkDescriptorSetLayout	_descSetLayout	{ VK_NULL_HANDLE };
		std::vector<std::pair<VkDescriptorSetLayoutBinding, VkDescriptorBindingFlags>> _descBindingInfos{};
	};
}

#endif