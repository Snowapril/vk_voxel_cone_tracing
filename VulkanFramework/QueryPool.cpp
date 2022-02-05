// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/QueryPool.h>
#include <iostream>

namespace vfs
{
	QueryPool::QueryPool(DevicePtr device, uint32_t numQuery)
	{
		assert(initialize(device, numQuery));
	}

	QueryPool::~QueryPool()
	{
		destroyQueryPool();
	}

	void QueryPool::destroyQueryPool(void)
	{
		vkDestroyQueryPool(_device->getDeviceHandle(), _queryPool, nullptr);
		_queryPool = VK_NULL_HANDLE;
		_numQuery = 0;
		_device.reset();
	}

	bool QueryPool::initialize(DevicePtr device, uint32_t numQuery)
	{
		_device = device;
		_numQuery = numQuery;

		VkQueryPoolCreateInfo queryPoolInfo = {};
		queryPoolInfo.sType				 = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolInfo.pNext				 = nullptr;
		queryPoolInfo.queryCount		 = numQuery;
		queryPoolInfo.queryType			 = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolInfo.pipelineStatistics = 0; // TODO(snowapril) : pipeline statistics
		queryPoolInfo.flags				 = 0;

		if (vkCreateQueryPool(_device->getDeviceHandle(), &queryPoolInfo, nullptr, &_queryPool) != VK_SUCCESS)
		{
			std::cerr << "[QueryPool] Failed to create query pool\n";
			return false;
		}

		return true;
	}

	bool QueryPool::readQueryResults(std::vector<uint64_t>* results)
	{
		results->resize(_numQuery);
		VkResult result = vkGetQueryPoolResults(_device->getDeviceHandle(), 
												_queryPool, 0, _numQuery, 
												sizeof(uint64_t) * _numQuery, 
												results->data(), sizeof(uint64_t), 
												VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_64_BIT);
		return result == VK_SUCCESS;
	}

	void QueryPool::writeTimeStamp(VkCommandBuffer cmdBuffer, VkPipelineStageFlagBits stageFlag, uint32_t queryIndex)
	{
		assert(queryIndex <= _numQuery);
		vkCmdWriteTimestamp(cmdBuffer, stageFlag, _queryPool, queryIndex);
	}

	void QueryPool::resetQueryPool(VkCommandBuffer cmdBuffer)
	{
		vkCmdResetQueryPool(cmdBuffer, _queryPool, 0, _numQuery);
	}
}