// Author : Jihong Shin (snowapril)

#if !defined(VFS_VOXELIZER_PASS_H)
#define VFS_VOXELIZER_PASS_H

#include <Common/pch.h>
#include <Common/EngineConfig.h>
#include <RenderPass/RenderPassBase.h>
#include <VoxelEngine/Counter.h>
#include <BoundingBox.h>
#include <ClipmapRegion.h>
#include <array>

namespace vfs
{
	struct FrameLayout;
	class GLTFScene;

	class Voxelizer : public RenderPassBase
	{
	public:
		explicit Voxelizer(DevicePtr device, uint32_t voxelResolution);
				~Voxelizer();

	public:
		Voxelizer& createAttachments	(VkExtent2D resolution);
		Voxelizer& createRenderPass		(void);
		Voxelizer& createFramebuffer	(VkExtent2D resolution);
		Voxelizer& createDescriptors	(void);
		Voxelizer& createVoxelClipmap	(uint32_t extentLevel0);
		
		void cmdVoxelize(VkCommandBuffer cmdBufferHandle, const ClipmapRegion& region, uint32_t clipLevel);
		
		inline VkExtent3D getClipmapResolution(void) const noexcept
		{
			return {
				getVoxelResolution() * DEFAULT_VOXEL_FACE_COUNT,
				getVoxelResolution() * DEFAULT_CLIP_REGION_COUNT,
				getVoxelResolution()
			};
		}
		inline uint32_t getVoxelResolution(void) const noexcept
		{
			// Add extra border to resolution for avoid bleeding with neighbor voxel
			return _voxelResolution + DEFAULT_VOXEL_BORDER;
		}
		inline DescriptorSetLayoutPtr getVoxelDescLayout(void) const
		{
			return _voxelDescLayout;
		}
		inline DescriptorSetPtr getVoxelDescSet(uint32_t clipmapLevel) const
		{
			assert(clipmapLevel < _voxelDescSets.size());
			return _voxelDescSets[clipmapLevel];
		}
	private:
		void onBeginRenderPass	(const FrameLayout* frameLayout) override;
		void onEndRenderPass	(const FrameLayout* frameLayout) override;
		void onUpdate			(const FrameLayout* frameLayout) override;
		void processWindowResize(int width, int height) override;
		void setViewport		(VkCommandBuffer cmdBufferHandle, glm::uvec3 viewportSize, uint32_t clipLevel);
		void setViewProjection	(VkCommandBuffer cmdBufferHandle, const ClipmapRegion& region, uint32_t clipLevel);

		struct VoxelizationDesc {
			glm::vec3	regionMinCorner;
			uint32_t	clipLevel;
			glm::vec3	regionMaxCorner;
			float		clipMaxExtent;
			glm::vec3	prevRegionMinCorner;
			float		voxelSize;
			glm::vec3	prevRegionMaxCorner;
			int32_t		clipmapResolution;
		};
	private:
		ImagePtr					_voxelOpacity			{ nullptr };
		ImageViewPtr				_voxelOpacityView		{ nullptr };
		ImagePtr					_voxelRadiance			{ nullptr };
		ImageViewPtr				_voxelRadianceView		{ nullptr };
		ImageViewPtr				_voxelRadianceR32View	{ nullptr };
		SamplerPtr					_voxelSampler			{ nullptr };
		FramebufferPtr				_framebuffer			{ nullptr };
		DescriptorPoolPtr			_voxelDescPool			{ nullptr };
		DescriptorSetLayoutPtr		_voxelDescLayout		{ nullptr };
		std::array<DescriptorSetPtr, DEFAULT_CLIP_REGION_COUNT> _voxelDescSets;
		std::array<BufferPtr,		 DEFAULT_CLIP_REGION_COUNT>	_viewProjBuffers;
		std::array<BufferPtr,		 DEFAULT_CLIP_REGION_COUNT>	_viewportBuffers;
		std::array<BufferPtr,		 DEFAULT_CLIP_REGION_COUNT>	_clipmapBuffers;
		VkSampleCountFlagBits		_sampleCount			{ VK_SAMPLE_COUNT_1_BIT };
		uint32_t					_voxelResolution		{ 0u };
	};
};

#endif