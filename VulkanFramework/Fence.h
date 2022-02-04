// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_FENCE)
#define VULKAN_FRAMEWORK_FENCE

#include <VulkanFramework/pch.h>
#include <memory>

namespace vfs
{
	class Device;

	class Fence : NonCopyable
	{
	public:
		explicit Fence() = default;
		explicit Fence(DevicePtr device, uint32_t numFences, VkFenceCreateFlags flags);
				~Fence();

	public:
		void destroyFence		(void);
		bool initialize			(DevicePtr device, uint32_t numFences, VkFenceCreateFlags flags);
		bool waitForFence		(uint32_t fenceIndex, uint64_t timeout);
		bool waitForAllFences	(uint64_t timeout);
		bool waitForAnyFences	(uint64_t timeout);
		bool resetFence			(void);

		inline const VkFence& getFence(uint32_t fenceIndex) const
		{
			assert(fenceIndex < _fences.size());
			return _fences[fenceIndex];
		}

	private:
		DevicePtr _device	{ nullptr };
		std::vector<VkFence>	_fences	{		  };
	};
}

#endif