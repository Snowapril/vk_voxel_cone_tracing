// Author : Jihong Shin (snowapril)

#if !defined(VFS_OCTREE_VOXEL_CONE_TRACING_H)
#define VFS_OCTREE_VOXEL_CONE_TRACING_H

#include <RenderPass/RenderPassBase.h>

namespace vfs
{
	class OctreeVoxelConeTracing : public RenderPassBase
	{
	public:
		explicit OctreeVoxelConeTracing(CommandPoolPtr cmdPool,
										VkExtent2D resolution);
				~OctreeVoxelConeTracing();

		enum class RenderingMode : uint32_t
		{
			Diffuse					= 0,
			Specular				= 1,
			Roughness				= 2,
			MinLevel				= 3,
			DirectContribution		= 4,
			IndirectDiffuse			= 5,
			IndirectSpecular		= 6,
			AmbientOcclusion		= 7,
			CombinedGI				= 8,
		};
	public:
		OctreeVoxelConeTracing& createAttachments		(void);
		OctreeVoxelConeTracing& createRenderPass		(void);
		OctreeVoxelConeTracing& createFramebuffer		(void);
		OctreeVoxelConeTracing& createDescriptors		(const BufferPtr& svo);
		OctreeVoxelConeTracing& createPipeline		(const DescriptorSetLayoutPtr& globalDescLayout);

		void drawGUI(void) override;
	private:
		void onBeginRenderPass	(const FrameLayout* frameLayout) override;
		void onEndRenderPass	(const FrameLayout* frameLayout) override;
		void onUpdate			(const FrameLayout* frameLayout) override;
		
		FramebufferAttachment createAttachment(VkExtent3D resolution, VkFormat format, VkImageUsageFlags usage);

		struct VoxelConeTracingDesc
		{
			glm::vec3 volumeCenter;			 // 12
			uint32_t renderingMode;			 // 16
			float voxelSize;				 // 20
			float volumeDimension;			 // 24
			float traceStartOffset; 		 // 28
			float indirectDiffuseIntensity;  // 32
			float ambientOcclusionFactor; 	 // 36
			float minTraceStepFactor; 		 // 40
			float indirectSpecularIntensity; // 44
			float occlusionDecay; 			 // 48
			int32_t enable32Cones;			 // 52
		};

	private:
		FramebufferPtr				_framebuffer				{ nullptr };
		DescriptorSetPtr			_descriptorSet				{ nullptr };
		DescriptorPoolPtr			_descriptorPool				{ nullptr };
		DescriptorSetLayoutPtr		_descriptorLayout			{ nullptr };
		DescriptorSetPtr			_gbufferDescriptorSet		{ nullptr };
		DescriptorSetLayoutPtr		_gbufferDescriptorLayout	{ nullptr };
		DescriptorSetPtr			_lightDescriptorSet			{ nullptr };
		DescriptorSetLayoutPtr		_lightDescriptorLayout		{ nullptr };
		SamplerPtr					_shadowSampler				{ nullptr };
		SamplerPtr					_colorSampler				{ nullptr };
		VkExtent2D					_resolution					{	0, 0  };

		// Rendering option parameters
		RenderingMode				_renderingMode		{ RenderingMode::CombinedGI };
		float						_traceStartOffset			{ 1.0f };
		float						_indirectDiffuseIntensity	{ 15.0f };
		float						_ambientOcclusionFactor		{ 0.5f };
		float						_minTraceStepFactor			{ 1.0f };
		float						_indirectSpecularIntensity	{ 3.0f };
		float						_occlusionDecay				{ 3.0f };
		bool						_enable32Cones				{ false };
	};
};

#endif