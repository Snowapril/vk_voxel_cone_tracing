// Author : Jihong Shin (rubikpril)

#if !defined(VOXEL_COUNTER)
#define VOXEL_COUNTER

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
		std::unique_ptr<Buffer>	_stagingBuffer			 { nullptr };
		// TODO(rubikpril) : do I need to store fence?
	};
}

#endif