// Author : Jihong Shin (snowapril)

#if !defined(VFS_CLIPMAP_CLEANER_H)
#define VFS_CLIPMAP_CLEANER_H

#include <pch.h>
#include <Util/EngineConfig.h>
#include <RenderPass/Clipmap/ClipmapRegion.h>
#include <VulkanFramework/Commands/CommandBuffer.h>

namespace vfs
{
	class ClipmapCleaner : NonCopyable
	{
	public:
		explicit ClipmapCleaner(DevicePtr device);
				~ClipmapCleaner();

	public:
		ClipmapCleaner&	createDescriptors		(const ImageView* opacityImageView,
												 const ImageView* radianceImageView,
												 const Sampler* clipmapSampler);
		ClipmapCleaner&	createPipeline			(void);
		void			destroyClipmapCleaner	(void);


		inline void cmdClearOpacityClipRegion(CommandBuffer cmdBuffer, const Image* image, glm::ivec3 regionMinCorner,
											  glm::uvec3 extent, const uint32_t clipLevel)
		{
			cmdClearImageClipmapRegion(cmdBuffer, image, regionMinCorner, extent, clipLevel, _opacityDescSet);
		}

		inline void cmdClearRadianceClipRegion(CommandBuffer cmdBuffer, const Image* image, glm::ivec3 regionMinCorner,
											   glm::uvec3 extent, const uint32_t clipLevel)
		{
			cmdClearImageClipmapRegion(cmdBuffer, image, regionMinCorner, extent, clipLevel, _radianceDescSet);
		}

	private:
		struct ImageCleaningDesc
		{
			glm::ivec3	regionMinCorner;    // 12
			int32_t     clipLevel;          // 16
			glm::uvec3	clipMaxExtent;      // 28
			int32_t     clipmapResolution;  // 32
			uint32_t    faceCount;          // 36
		};

		void cmdClearImageClipmapRegion(CommandBuffer cmdBuffer, const Image* image, glm::ivec3 regionMinCorner,
										glm::uvec3 extent, const uint32_t clipLevel, const DescriptorSetPtr& descSet);

	private:
		DevicePtr					_device				{ nullptr };
		DescriptorPoolPtr			_descPool			{ nullptr };
		DescriptorSetLayoutPtr		_descLayout			{ nullptr };
		PipelineLayoutPtr			_pipelineLayout		{ nullptr };
		ComputePipelinePtr			_pipeline			{ nullptr };
		DescriptorSetPtr			_radianceDescSet	{ nullptr };
		DescriptorSetPtr			_opacityDescSet		{ nullptr };
	};
};

#endif