// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Buffers/Buffer.h>

namespace vfs
{
	Buffer::Buffer(VmaAllocator allocator, uint64_t bufferSize, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage)
	{
		assert(initialize(allocator, bufferSize, bufferUsage, memoryUsage));
	}

	Buffer::~Buffer()
	{
		destroyBuffer();
	}

	void Buffer::destroyBuffer(void)
	{
		if (_allocator != nullptr && _buffer != VK_NULL_HANDLE && _bufferAllocation != nullptr)
		{
			vmaDestroyBuffer(_allocator, _buffer, _bufferAllocation);
			_allocator			= nullptr;
			_buffer				= VK_NULL_HANDLE;
			_bufferAllocation	= nullptr;
		}
	}

	bool Buffer::initialize(VmaAllocator allocator, uint64_t bufferSize, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage)
	{
		_allocator = allocator;
		_allocatedSize = bufferSize;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.pNext = nullptr;
		bufferInfo.size = bufferSize;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferInfo.usage = bufferUsage;

		VmaAllocationCreateInfo bufferAllocInfo = {};
		bufferAllocInfo.usage = memoryUsage;

		if (vmaCreateBuffer(_allocator, &bufferInfo, &bufferAllocInfo, &_buffer, &_bufferAllocation, nullptr) != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}

	void Buffer::uploadData(const void* srcData, uint64_t size)
	{
		assert(srcData != nullptr); // snowapril : source data must not be invalid
		void* dstData{ nullptr };
		vmaMapMemory(_allocator, _bufferAllocation, &dstData);
		memcpy(dstData, srcData, static_cast<size_t>(size));
		vmaUnmapMemory(_allocator, _bufferAllocation);
	}

	void Buffer::downloadData(void* dstData, uint64_t size)
	{
		void* srcData{ nullptr };
		vmaMapMemory(_allocator, _bufferAllocation, &srcData);
		memcpy(dstData, srcData, static_cast<size_t>(size));
		vmaUnmapMemory(_allocator, _bufferAllocation);
	}

	VkBufferMemoryBarrier Buffer::generateMemoryBarrier(VkAccessFlags srcMask, VkAccessFlags dstMask)
	{
		return generateMemoryBarrier(srcMask, dstMask, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED);
	}

	VkBufferMemoryBarrier Buffer::generateMemoryBarrier(VkAccessFlags srcMask, VkAccessFlags dstMask,
														uint32_t srcQueueFamily, uint32_t dstQueueFamily)
	{
		VkBufferMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.pNext = nullptr;
		barrier.offset = 0;
		barrier.size = _allocatedSize;
		barrier.buffer = _buffer;
		barrier.srcQueueFamilyIndex = srcQueueFamily;
		barrier.srcAccessMask = srcMask;
		barrier.dstQueueFamilyIndex = dstQueueFamily;
		barrier.dstAccessMask = dstMask;
		return barrier;
	}
};