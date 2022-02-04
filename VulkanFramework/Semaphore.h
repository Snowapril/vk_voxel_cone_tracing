// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_SEMAPHORE)
#define VULKAN_FRAMEWORK_SEMAPHORE

#include <VulkanFramework/pch.h>

namespace vfs
{
	class Device;

	class Semaphore : NonCopyable
	{
	public:
		explicit Semaphore() = default;
		explicit Semaphore(DevicePtr device);
				~Semaphore();

	public:
		void destroySemaphore(void);
		bool initialize(DevicePtr device);

		inline const VkSemaphore& getHandle(void) const
		{
			return _semaphore;
		}

	private:
		DevicePtr	_device		{ nullptr };
		VkSemaphore _semaphore	{ VK_NULL_HANDLE };
	};
}

#endif