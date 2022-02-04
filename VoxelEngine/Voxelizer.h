// Author : Jihong Shin (rubikpril)

#if !defined(VOXEL_ENGINE_VOXELIZER_H)
#define VOXEL_ENGINE_VOXELIZER_H

#include <memory>
#include <VulkanFramework/Framebuffer.h>
#include <VulkanFramework/CommandBuffer.h>
#include <VulkanFramework/DescriptorSet.h>
#include <VulkanFramework/GraphicsPipeline.h>

namespace vfs
{
	class CommandPool;
	class Device;
	class Scene;
	class Counter;
	class DescriptorPool;
	class DescriptorSetLayout;

	class SparseVoxelizer
	{
	public:
		explicit SparseVoxelizer() = default;
		explicit SparseVoxelizer(DevicePtr device, std::shared_ptr<Scene> scene, const uint32_t octreeLevel);
				~SparseVoxelizer();

	public:
		void destroyVoxelizer	(void);
		bool initialize			(DevicePtr device, std::shared_ptr<Scene> scene, const uint32_t octreeLevel);
		void preVoxelize		(const CommandPoolPtr& cmdPool);
		void cmdVoxelize		(CommandBuffer cmdBuffer);
		void readVoxelFragments	(const CommandPoolPtr& cmdPool, OUT std::vector<glm::uvec3>* readData);

		inline BufferPtr getFramgnetList(void) const
		{
			return _fragmentList;
		}
		inline uint32_t getLevel(void) const
		{
			return _level;
		}
		inline uint32_t getVoxelResolution(void) const
		{
			return _voxelResolution;
		}
		inline uint32_t getFragmentCount(void) const
		{
			return _voxelFragmentCount;
		}
	private:
		bool initializeRenderPass		(void);
		bool initializeDescriptors		(void);
		bool initializePipelineLayout	(void);
		bool initializePipeline			(VkRenderPass renderPass);

	private:
		std::unique_ptr<Counter>			  _counter				{ nullptr };
		BufferPtr				  _fragmentList			{ nullptr };
		std::unique_ptr<Framebuffer>		  _framebuffer			{ nullptr };
		std::shared_ptr<GraphicsPipeline>	  _pipeline				{ nullptr };
		DevicePtr				  _device				{ nullptr };
		std::shared_ptr<Scene>				  _scene				{ nullptr };
		VkPipelineLayout					  _pipelineLayout		{ VK_NULL_HANDLE };
		std::shared_ptr<DescriptorPool>		  _descriptorPool		{ nullptr };
		std::shared_ptr<DescriptorSetLayout>  _descriptorLayout		{ nullptr };
		std::unique_ptr<DescriptorSet>		  _descriptorSet		{ nullptr };
		VkRenderPass						  _renderPass			{ VK_NULL_HANDLE };
		uint32_t							  _voxelResolution		{ 0 };
		uint32_t							  _voxelFragmentCount	{ 0 };
		uint32_t							  _level				{ 0 };
	};
}

#endif