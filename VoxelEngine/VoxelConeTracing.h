// Author : Jihong Shin (rubikpril)

#if !defined(RENDER_ENGINE_VOXEL_CONE_TRACING_H)
#define RENDER_ENGINE_VOXEL_CONE_TRACING_H

#include <VoxelEngine/pch.h>
#include <VulkanFramework/GraphicsPipeline.h>
#include <memory>

namespace vfs
{
	struct FrameLayout;
	class Device;

	class VoxelConeTracing : NonCopyable
	{
	public:
		explicit VoxelConeTracing() = default;
		explicit VoxelConeTracing(DevicePtr device, const RenderPassPtr& renderPass);
				~VoxelConeTracing();

	public:
		void destroyVoxelConeTraicng(void);
		bool initialize				(DevicePtr device, const RenderPassPtr& renderPass);
		void render					(const FrameLayout* frame);

	private:
		bool initializePipelineLayout(void);
		bool initializePipeline		 (const RenderPassPtr& renderPass);

	private:
		DevicePtr				_device			{	 nullptr	 };
		std::unique_ptr<GraphicsPipeline>	_pipeline		{	 nullptr	 };
		VkPipelineLayout					_pipelineLayout	{ VK_NULL_HANDLE };
	};
}

#endif