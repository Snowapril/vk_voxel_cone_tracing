// Author : Jihong Shin (snowapril)

#if !defined(VFS_VOXELIZATION_PASS_H)
#define VFS_VOXELIZATION_PASS_H

#include <Common/pch.h>
#include <Common/EngineConfig.h>
#include <RenderPass/RenderPassBase.h>
#include <VoxelEngine/Counter.h>
#include <BoundingBox.h>
#include <ClipmapRegion.h>
#include <array>
#include <VoxelEngine/Counter.h>

namespace vfs
{
	class GLTFScene;
	class Voxelizer;

	class VoxelizationPass : public RenderPassBase
	{
	public:
		explicit VoxelizationPass(DevicePtr device, uint32_t voxelResolution, std::shared_ptr<GLTFScene> scene);
				~VoxelizationPass();

	public:
		VoxelizationPass& initialize			(void);
		VoxelizationPass& createVoxelClipmap	(uint32_t extentLevel0);
		VoxelizationPass& createDescriptors		(void);
		VoxelizationPass& createPipeline		(const DescriptorSetLayoutPtr& globalDescLayout);
		
		void drawUI(void) override;
		
		BufferPtr _imageCoordBuffer;
	private:
		void onBeginRenderPass	(const FrameLayout* frameLayout) override;
		void onEndRenderPass	(const FrameLayout* frameLayout) override;
		void onUpdate			(const FrameLayout* frameLayout) override;

		glm::ivec3 calculateChangeDelta(const uint32_t clipLevel, const BoundingBox<glm::vec3>& cameraBB);

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
		std::shared_ptr<GLTFScene> _scene;
		VkSampleCountFlagBits	_sampleCount{ VK_SAMPLE_COUNT_1_BIT };

		// TODO(snowapril) : need to remove this
		std::unique_ptr<Counter> _counter;
	};
};

#endif