// Author : Jihong Shin (snowapril)

#if !defined(VOXEL_ENGINE_OCTREE_VOXEL_CONE_TRACING_H)
#define VOXEL_ENGINE_OCTREE_VOXEL_CONE_TRACING_H

#include <RenderPass/RenderPassBase.h>

namespace vfs
{
	struct FrameLayout;

	class OctreeVoxelConeTracing : public RenderPassBase
	{
	public:
		explicit OctreeVoxelConeTracing(DevicePtr device,
									  RenderPassPtr renderPass, 
									  VkExtent2D resolution);
				~OctreeVoxelConeTracing();

		enum class RenderingMode : uint32_t
		{
			Diffuse					= 0,
			Specular				= 1,
			Roughness				= 2,
			DirectContribution		= 3,
			IndirectContribution	= 4,
			AmbientOcclusion		= 5,
			CombinedGI				= 6,
		};
	public:
		OctreeVoxelConeTracing& createDescriptors	(void);
		OctreeVoxelConeTracing& createPipeline		(const DescriptorSetLayoutPtr& globalDescLayout);

		void drawUI		(void) override;
		void cmdTrace	(const FrameLayout* frameLayout);

		inline void setRenderingMode(RenderingMode mode)
		{
			_renderingMode = mode;
		}
	private:
		void onBeginRenderPass	(const FrameLayout* frameLayout) override;
		void onEndRenderPass	(const FrameLayout* frameLayout) override;
		void onUpdate			(const FrameLayout* frameLayout) override;
		struct VoxelConeTracingDesc
		{
			glm::vec3 volumeCenter;
			float voxelSize;
			float volumeDimension;
		};

	private:
		DescriptorSetPtr			_descriptorSet				{ nullptr };
		DescriptorPoolPtr			_descriptorPool				{ nullptr };
		DescriptorSetLayoutPtr		_descriptorLayout			{ nullptr };
		DescriptorSetPtr			_gbufferDescriptorSet		{ nullptr };
		DescriptorSetLayoutPtr		_gbufferDescriptorLayout	{ nullptr };
		DescriptorSetPtr			_lightDescriptorSet			{ nullptr };
		DescriptorSetLayoutPtr		_lightDescriptorLayout		{ nullptr };
		SamplerPtr					_shadowSampler				{ nullptr };
		BufferPtr					_vctDescBuffer				{ nullptr };
		VkExtent2D					_resolution					{	0, 0  };
		RenderingMode				_renderingMode;
	};
};

#endif