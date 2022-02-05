// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_BUFFER_VIEW_H)
#define VULKAN_FRAMEWORK_BUFFER_VIEW_H

#include <VulkanFramework/pch.h>

namespace vfs
{
	class BufferView : NonCopyable
	{
	public:
		explicit BufferView() = default;
		explicit BufferView(DevicePtr device, BufferPtr buffer, VkFormat format);
		~BufferView();

	public:
		void destroyBufferView(void);
		bool initialize(DevicePtr device, BufferPtr buffer, VkFormat format);

		inline VkBufferView getBufferViewHandle(void) const
		{
			return _bufferViewHandle;
		}
		inline BufferPtr getBufferPtr(void) const
		{
			return _buffer;
		}

	private:
		DevicePtr		_device				{	nullptr		 };
		BufferPtr		_buffer				{	nullptr		 };
		VkBufferView	_bufferViewHandle	{ VK_NULL_HANDLE };
	};
}

#endif