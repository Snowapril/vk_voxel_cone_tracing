// Author : Jihong Shin (rubikpril)

#if !defined(VOXEL_ENGINE_OCTREE_VISUALIZER)
#define VOXEL_ENGINE_OCTREE_VISUALIZER

#include <VoxelEngine/pch.h>
#include <VulkanFramework/ComputePipeline.h>
#include <VulkanFramework/GraphicsPipeline.h>

namespace vfs
{
	struct FrameLayout;
	class OctreeBuilder;
	class Device;

	class OctreeVisualizer : NonCopyable
	{
	public:
		explicit OctreeVisualizer() = default;
		// TODO(rubikpril) : replace octree builder to svo class, VkRenderPass to RenderPassPtr
		explicit OctreeVisualizer(DevicePtr device, CameraPtr camera, std::shared_ptr<OctreeBuilder> builder, VkRenderPass renderPass);
				~OctreeVisualizer();

	public:
		void destroyOctreeVisualizer(void);
		bool initialize				(DevicePtr device, CameraPtr camera, std::shared_ptr<OctreeBuilder> builder, VkRenderPass renderPass);
		void buildPointClouds		(const CommandPoolPtr& cmdPool);
		void cmdDraw				(const FrameLayout* frame);

	private:
		bool initializeBuffers			(void);
		bool initializePipelineLayout	(void);
		bool initializePipelines		(VkRenderPass renderPass);

	private:
		std::shared_ptr<OctreeBuilder>		 _octreeBuilder			{ nullptr };
		DevicePtr							 _device				{ nullptr };
		BufferPtr							 _octreeVertexBuffer	{ nullptr };
		CameraPtr							 _camera				{ nullptr };
		std::shared_ptr<GraphicsPipeline>	 _avcPipeline			{ nullptr };
		VkPipelineLayout					 _pipelineLayout		{ VK_NULL_HANDLE };
		std::vector<glm::uvec2>				 _pointClouds;
	};
}

#endif