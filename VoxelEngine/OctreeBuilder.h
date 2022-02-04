// Author : Jihong Shin (rubikpril)

#if !defined(VOXEL_ENGINE_OCTREE_BUILDER)
#define VOXEL_ENGINE_OCTREE_BUILDER

#include <VoxelEngine/pch.h>
#include <VulkanFramework/CommandBuffer.h>
#include <VulkanFramework/ComputePipeline.h>

namespace vfs
{
	class Counter;
	class SparseVoxelizer;

	class OctreeBuilder : NonCopyable
	{
	public:
		explicit OctreeBuilder() = default;
		explicit OctreeBuilder(DevicePtr device, std::shared_ptr<SparseVoxelizer> voxelizer);
				~OctreeBuilder();

	public:
		void destroyOctreeBuilder	(void);
		bool initialize				(DevicePtr device, std::shared_ptr<SparseVoxelizer> voxelizer);
		void cmdBuild				(CommandBuffer cmdBuffer);
		void transferOwnership		(CommandBuffer cmdBuffer, VkPipelineStageFlags srcStage, 
									 VkPipelineStageFlags dstStage, uint32_t srcQueueFamily, 
									 uint32_t dstQueueFamily);
		void readOctreeNodes		(const CommandPoolPtr& cmdPool, OUT std::vector<uint32_t>* readData);
		
		inline uint32_t getOctreeLevel(void) const
		{
			return _level;
		}
		inline uint32_t getNumOctreeNodes(void) const
		{
			return _numOctreeNodes;
		}
		inline BufferPtr getOctreeBuffer(void) const
		{
			return _octreeBuffer;
		}
	private:
		bool initializeBuffers			(void);
		bool initializeDescriptors		(void);
		bool initializePipelineLayout	(void);
		bool initializePipelines		(void);

	private:
		std::shared_ptr<Counter>		_octreeNodeCounter;
		std::shared_ptr<SparseVoxelizer>		_voxelizer;
		DevicePtr						_device;
		ComputePipelinePtr				_nodeInitPipeline;
		ComputePipelinePtr				_nodeFlagPipeline;
		ComputePipelinePtr				_nodeAllocPipeline;
		ComputePipelinePtr				_nodeModifyArgPipeline;
		BufferPtr						_infoBuffer;
		BufferPtr						_infoStagingBuffer;
		BufferPtr						_indirectBuffer;
		BufferPtr						_indirectStagingBuffer;
		BufferPtr						_octreeBuffer;
		DescriptorPoolPtr				_descriptorPool		{ nullptr };
		DescriptorSetLayoutPtr			_descriptorLayout	{ nullptr };
		std::unique_ptr<DescriptorSet>	_descriptorSet		{ nullptr };
		VkPipelineLayout				_pipelineLayout		{ VK_NULL_HANDLE };
		uint32_t						_numOctreeNodes		{ 0 };
		uint32_t						_level				{ 0 };
	};
}

#endif