// Author : Jihong Shin (snowapril)

#if !defined(VFS_DOWNSAMPLER_H)
#define VFS_DOWNSAMPLER_H

#include <pch.h>
#include <Util/EngineConfig.h>
#include <RenderPass/Clipmap/ClipmapRegion.h>
#include <VulkanFramework/Commands/CommandBuffer.h>

namespace vfs
{
	struct FrameLayout;
	class GLTFScene;

	class DownSampler : NonCopyable
	{
	public:
		explicit DownSampler(DevicePtr device);
				~DownSampler();

	public:
		DownSampler& createDescriptors		(const ImageView* opacityImageView, 
											 const ImageView* radianceImageView, 
											 const Sampler* sampler);
		DownSampler& createPipeline			(void);
		void		 destroyDownSampler		(void);

		inline void	cmdDownSampleOpacity	(CommandBuffer cmdBuffer, const Image* image,
											 const std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>& clipRegions,
											 uint32_t clipLevel)
		{
			cmdDownSample(cmdBuffer, image, clipRegions, clipLevel, DownSampleMode::OpacityMode);
		}
		inline void	cmdDownSampleRadiance	(CommandBuffer cmdBuffer, const Image* image,
											 const std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>& clipRegions,
											 uint32_t clipLevel)
		{
			cmdDownSample(cmdBuffer, image, clipRegions, clipLevel, DownSampleMode::RadianceMode);
		}
	private:
		enum class DownSampleMode : unsigned char
		{
			OpacityMode		= 0,
			RadianceMode	= 1,
			Last			= 2,
		};

		struct DownSampleDesc
		{
			glm::ivec3 	prevRegionMinCorner;	// 12
			int 		clipLevel;				// 16
			int 		clipmapResolution;		// 20
			int 		downSampleRegionSize;	// 24
		};

		void cmdDownSample(CommandBuffer cmdBuffer, const Image* image,
						   const std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>& clipRegions,
						   uint32_t clipLevel, DownSampleMode mode);

	private:
		DevicePtr					_device						{ nullptr };
		DescriptorPoolPtr			_descPool					{ nullptr };
		DescriptorSetLayoutPtr		_descLayout					{ nullptr };
		PipelineLayoutPtr			_pipelineLayout				{ nullptr };
		ComputePipelinePtr			_opacityDownSamplePipeline	{ nullptr };
		ComputePipelinePtr			_radianceDownSamplePipeline	{ nullptr };
		std::array<BufferPtr,			DEFAULT_CLIP_REGION_COUNT - 1> _downSampleDescBuffer		{ nullptr };
		std::array<DescriptorSetPtr,	DEFAULT_CLIP_REGION_COUNT - 1> _opacityDownSampleDescSet	{ nullptr };
		std::array<DescriptorSetPtr,	DEFAULT_CLIP_REGION_COUNT - 1> _radianceDownSampleDescSet	{ nullptr };
	};
};

#endif