// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Buffers/Buffer.h>
#include <VulkanFramework/Buffers/BufferView.h>
#include <VulkanFramework/Device.h>

namespace vfs
{
	BufferView::BufferView(DevicePtr device, BufferPtr buffer, VkFormat format)
	{
		assert(initialize(device, buffer, format));
	}

	BufferView::~BufferView()
	{
		destroyBufferView();
	}

	void BufferView::destroyBufferView(void)
	{
		if (_bufferViewHandle != VK_NULL_HANDLE)
		{
			vkDestroyBufferView(_device->getDeviceHandle(), _bufferViewHandle, nullptr);
			_bufferViewHandle = VK_NULL_HANDLE;
		}
		_device.reset();
		_buffer.reset();
	}

	bool BufferView::initialize(DevicePtr device, BufferPtr buffer, VkFormat format)
	{
		_device = device;
		_buffer = buffer;

		VkBufferViewCreateInfo viewInfo = {};
		viewInfo.sType	= VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
		viewInfo.pNext	= nullptr;
		viewInfo.offset = 0;
		viewInfo.buffer = _buffer->getBufferHandle();
		viewInfo.format = format;
		viewInfo.range	= _buffer->getTotalSize();

		return vkCreateBufferView(_device->getDeviceHandle(), &viewInfo, nullptr, &_bufferViewHandle)
			== VK_SUCCESS;
	}
}