// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_DESCRIPTOR_SET_H)
#define VULKAN_FRAMEWORK_DESCRIPTOR_SET_H

#include <VulkanFramework/pch.h>
#include <memory>

namespace vfs
{
	class Buffer;
	class Device;
	class DescriptorPool;
	class DescriptorSetLayout;

	class DescriptorSet : NonCopyable
	{
	public:
		explicit DescriptorSet() = default;
		explicit DescriptorSet(DevicePtr device,
							   DescriptorPoolPtr descriptorPool,
							   DescriptorSetLayoutPtr descriptorSetLayout,
							   const uint32_t descSetCount);
				~DescriptorSet();

	public:
		bool initialize				(DevicePtr device,
									 DescriptorPoolPtr descriptorPool,
									 DescriptorSetLayoutPtr descriptorSetLayout,
									 const uint32_t descSetCount);
		void destroyDescriptorSet	(void);

		void updateImage			(const std::vector<VkDescriptorImageInfo>& imageInfos,
									 const uint32_t dstBinding,
									 VkDescriptorType descType);

		inline void updateStorageBuffer(const std::vector<BufferPtr>& buffers, 
										const uint32_t dstBinding, 
										const uint32_t descCount)
		{
			updateBuffer(buffers, dstBinding, descCount, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		}

		inline void updateStorageTexelBuffer(const BufferViewPtr& bufferView,
											 const uint32_t dstBinding, 
											 const uint32_t descCount)
		{
			updateTexelBuffer(bufferView, dstBinding, descCount, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER);
		}

		inline void updateUniformBuffer(const std::vector<BufferPtr>& buffers,
										const uint32_t dstBinding, 
										const uint32_t descCount)
		{
			updateBuffer(buffers, dstBinding, descCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		}

		inline VkDescriptorSet getHandle(void) const
		{
			return _descriptorSet;
		}

	private:
		void updateBuffer		(const std::vector<BufferPtr>& buffers, 
								 const uint32_t dstBinding, 
								 const uint32_t descCount,
								 VkDescriptorType descType);
		void updateTexelBuffer	(const BufferViewPtr& bufferView, 
								 const uint32_t dstBinding, 
								 const uint32_t descCount,
								 VkDescriptorType descType);
	private:
		DevicePtr				_device;
		DescriptorPoolPtr		_descriptorPool;
		DescriptorSetLayoutPtr  _descriptorSetLayout;
		VkDescriptorSet			_descriptorSet	{ VK_NULL_HANDLE };
	};
}

#endif