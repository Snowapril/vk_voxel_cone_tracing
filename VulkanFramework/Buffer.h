// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_BUFFER_H)
#define VULKAN_FRAMEWORK_BUFFER_H

#include <VulkanFramework/pch.h>

namespace vfs
{
	class Buffer : NonCopyable
	{
	public:
		explicit Buffer() = default;
		explicit Buffer(VmaAllocator allocator, uint64_t bufferSize, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage);
				~Buffer();

	public:
		void destroyBuffer	(void);
		bool initialize		(VmaAllocator allocator, uint64_t bufferSize, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage);
		void uploadData		(const void* srcData, uint64_t size);
		void downloadData	(void* dstData, uint64_t size);
		VkBufferMemoryBarrier generateMemoryBarrier(VkAccessFlags srcMask, VkAccessFlags dstMask);
		VkBufferMemoryBarrier generateMemoryBarrier(VkAccessFlags srcMask, VkAccessFlags dstMask, 
													uint32_t srcQueueFamily, uint32_t dstQueueFamily);
		inline VkBuffer getBufferHandle(void) const
		{
			return _buffer;
		}
		inline uint64_t getTotalSize(void) const
		{
			return _allocatedSize;
		}

	private:
		VmaAllocator	_allocator			{	nullptr		 };
		VkBuffer		_buffer				{ VK_NULL_HANDLE };
		VmaAllocation	_bufferAllocation	{	nullptr		 };
		uint64_t		_allocatedSize		{ 0 };
	};
}

#endif