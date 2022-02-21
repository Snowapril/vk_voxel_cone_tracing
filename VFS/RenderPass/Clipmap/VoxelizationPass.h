// Author : Jihong Shin (snowapril)

#if !defined(VFS_VOXELIZATION_PASS_H)
#define VFS_VOXELIZATION_PASS_H

#include <pch.h>
#include <Util/EngineConfig.h>
#include <RenderPass/RenderPassBase.h>
#include <RenderPass/Clipmap/ClipmapRegion.h>
#include <Counter.h>
#include <BoundingBox.h>
#include <array>

namespace vfs
{
	class Voxelizer;

	class VoxelizationPass : public RenderPassBase
	{
	public:
		explicit VoxelizationPass(CommandPoolPtr cmdPool, uint32_t voxelResolution);
		explicit VoxelizationPass(CommandPoolPtr cmdPool, uint32_t voxelResolution, 
								  uint32_t voxelExtentLevel0, const DescriptorSetLayoutPtr& globalDescLayout);
				~VoxelizationPass();

	public:
		bool initialize			(uint32_t voxelResolution, uint32_t voxelExtentLevel0,
								 const DescriptorSetLayoutPtr& globalDescLayout);

		VoxelizationPass& createVoxelClipmap	(uint32_t extentLevel0);
		VoxelizationPass& createDescriptors		(void);
		VoxelizationPass& createPipeline		(const DescriptorSetLayoutPtr& globalDescLayout);
		
		void createOpacityVoxelSlice(void);
		void updateOpacityVoxelSlice(void);

		void drawGUI		(void) override;
		void drawDebugInfo	(void) override;
	private:
		void onBeginRenderPass	(const FrameLayout* frameLayout) override;
		void onEndRenderPass	(const FrameLayout* frameLayout) override;
		void onUpdate			(const FrameLayout* frameLayout) override;

		glm::ivec3  calculateChangeDelta	 (const uint32_t clipLevel, const BoundingBox<glm::vec3>& cameraBB);
		void		fillRevoxelizationRegions(const uint32_t clipLevel, const BoundingBox<glm::vec3>& boundingBox);

	private:
		Voxelizer*				_voxelizer			{ nullptr };
		Image*					_voxelOpacity		{ nullptr };
		ImageView*				_voxelOpacityView	{ nullptr };
		Sampler*				_voxelSampler		{ nullptr };
		DescriptorPoolPtr		_descriptorPool;
		DescriptorSetPtr		_descriptorSet;
		DescriptorSetLayoutPtr	_descriptorLayout;
		std::array<ClipmapRegion,				DEFAULT_CLIP_REGION_COUNT> _clipmapRegions;
		std::array<std::vector<ClipmapRegion>,	DEFAULT_CLIP_REGION_COUNT> _revoxelizationRegions;
		std::array<int32_t, DEFAULT_CLIP_REGION_COUNT> _clipMinChange{ 2, 2, 2, 2, 2, 1 };
		uint32_t				_voxelResolution;
		bool					_fullRevoxelization	{ true };

		std::vector<std::pair<ImagePtr, ImageViewPtr>> _opacitySlice;
		std::vector<VkDescriptorSet> _opacitySliceDescSet;
		SamplerPtr _opacitySliceSampler;
	};
};

#endif