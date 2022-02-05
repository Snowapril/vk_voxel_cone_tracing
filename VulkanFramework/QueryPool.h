// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_GPU_PROFILER)
#define VULKAN_FRAMEWORK_GPU_PROFILER

#include <VulkanFramework/pch.h>
#include <memory>

namespace vfs
{
	class Device;
	class Counter;
	class PipelineBase;

	class QueryPool
	{
	public:
		explicit QueryPool() = default;
		explicit QueryPool(DevicePtr device, uint32_t numQuery);
				~QueryPool();

	public:
		void destroyQueryPool	(void);
		bool initialize			(DevicePtr device, uint32_t numQuery);
		bool readQueryResults	(std::vector<uint64_t>* results);
		void writeTimeStamp(VkCommandBuffer cmdBuffer, VkPipelineStageFlagBits stageFlag, uint32_t queryIndex);
		void resetQueryPool(VkCommandBuffer cmdBuffer);

		inline uint32_t getNumQuery(void) const
		{
			return _numQuery;
		}
		inline VkQueryPool getHandle(void) const
		{
			return _queryPool;
		}

	private:
		DevicePtr	_device				{	nullptr		 };
		VkQueryPool	_queryPool			{ VK_NULL_HANDLE };
		uint32_t	_numQuery			{		0		 };
	};
}

#endif