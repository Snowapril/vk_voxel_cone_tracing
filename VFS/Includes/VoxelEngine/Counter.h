// Author : Jihong Shin (snowapril)

#if !defined(VOXEL_COUNTER)
#define VOXEL_COUNTER

#include <Common/pch.h>
#include <VulkanFramework/Buffer.h>
#include <memory>

namespace vfs
{
	class CommandPool;
	class Device;

	class Counter : NonCopyable
	{
	public:
		explicit Counter() = default;
		explicit Counter(DevicePtr device);
				~Counter();

	public:
		void	 destroyCounter(void);
		bool	 initialize					(DevicePtr device);
		uint32_t readCounterValue			(const CommandPoolPtr& cmdPool);
		void	 resetCounter				(const CommandPoolPtr& cmdPool);

		inline BufferPtr getBuffer(void) const
		{
			return _buffer;
		}

	private:
		DevicePtr				_device					 { nullptr };
		BufferPtr				_buffer					 { nullptr };
		BufferPtr				_stagingBuffer			 { nullptr };
		// TODO(snowapril) : do I need to store fence?
	};
}

#endif