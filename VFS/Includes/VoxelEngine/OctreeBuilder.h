// Author : Jihong Shin (snowapril)

#if !defined(VOXEL_ENGINE_OCTREE_BUILDER)
#define VOXEL_ENGINE_OCTREE_BUILDER

#include <Common/pch.h>
#include <VulkanFramework/Pipelines/ComputePipeline.h>

namespace vfs
{
	class Counter;
	class SparseVoxelizer;

	class OctreeBuilder : NonCopyable
	{
	public:
		explicit OctreeBuilder() = default;
		explicit OctreeBuilder(DevicePtr device, const CommandPoolPtr& cmdPool, std::shared_ptr<SparseVoxelizer> voxelizer);
				~OctreeBuilder();

	public:
		void destroyOctreeBuilder	(void);
		bool initialize				(DevicePtr device, const CommandPoolPtr& cmdPool, std::shared_ptr<SparseVoxelizer> voxelizer);
		void cmdBuild				(VkCommandBuffer cmdBuffer);
		void transferOwnership		(VkCommandBuffer cmdBuffer, VkPipelineStageFlags srcStage, 
									 VkPipelineStageFlags dstStage, uint32_t srcQueueFamily, 
									 uint32_t dstQueueFamily);
		void readOctreeNodes		(const CommandPoolPtr& cmdPool, OUT std::vector<glm::uvec2>* readData);
		void readOctreeBricks		(const CommandPoolPtr& cmdPool, OUT std::vector<uint32_t>* readData);

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
		inline BufferPtr getOctreeBrickImage(void) const
		{
			return _octreeBrickImageBuffer;
		}
	private:
		bool initializeBuffers			(const CommandPoolPtr& cmdPool);
		bool initializeDescriptors		(void);
		bool initializePipelineLayout	(void);
		bool initializePipelines		(void);
		void cmdNodeSubdivision			(VkCommandBuffer cmdBufferHandle);
		void cmdMipmapWrite				(VkCommandBuffer cmdBufferHandle);

	private:
		std::shared_ptr<SparseVoxelizer>	_voxelizer;
		DevicePtr					_device;
		ComputePipelinePtr			_nodeInitPipeline;
		ComputePipelinePtr			_nodeFlagPipeline;
		ComputePipelinePtr			_nodeAllocPipeline;
		ComputePipelinePtr			_nodeModifyArgPipeline;
		ComputePipelinePtr			_nodeLeafWritePipeline;
		ComputePipelinePtr			_nodeMipmapWritePipeline;
		BufferPtr					_infoBuffer;
		BufferPtr					_infoStagingBuffer;
		BufferPtr					_indirectBuffer;
		BufferPtr					_indirectStagingBuffer;
		BufferPtr					_octreeBuffer;
		std::shared_ptr<Counter>	_octreeNodeCounter;
		BufferPtr					_octreeBrickImageBuffer;
		BufferViewPtr				_octreeBrickImageBufferView;
		std::shared_ptr<Counter>	_octreeBrickCounter;
		DescriptorPoolPtr			_descriptorPool		{ nullptr };
		DescriptorSetLayoutPtr		_descriptorLayout	{ nullptr };
		DescriptorSetPtr			_descriptorSet		{ nullptr };
		PipelineLayoutPtr			_pipelineLayout		{ nullptr };
		uint32_t					_numOctreeNodes		{ 0 };
		uint32_t					_numOctreeBricks	{ 0 };
		uint32_t					_level				{ 0 };
	};
}

#endif