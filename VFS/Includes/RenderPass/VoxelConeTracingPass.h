// Author : Jihong Shin (snowapril)

#if !defined(VFS_VOXEL_CONE_TRACING_PASS_H)
#define VFS_VOXEL_CONE_TRACING_PASS_H

#include <RenderPass/RenderPassBase.h>

namespace vfs
{
	class VoxelConeTracingPass : public RenderPassBase
	{
	public:
		explicit VoxelConeTracingPass(DevicePtr device,
									  RenderPassPtr renderPass, 
									  VkExtent2D resolution);
				~VoxelConeTracingPass();

		enum class RenderingMode : uint32_t
		{
			Diffuse					= 0,
			Specular				= 1,
			Roughness				= 2,
			MinLevel				= 3,
			DirectContribution		= 4,
			IndirectContribution	= 5,
			AmbientOcclusion		= 6,
			CombinedGI				= 7,
		};
	public:
		VoxelConeTracingPass& createDescriptors		(void);
		VoxelConeTracingPass& createPipeline		(const DescriptorSetLayoutPtr& globalDescLayout);

		void drawUI(void) override;

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
		RenderingMode				_renderingMode		{ RenderingMode::CombinedGI };
	};
};

#endif