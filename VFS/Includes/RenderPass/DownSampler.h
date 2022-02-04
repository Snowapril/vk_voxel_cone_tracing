// Author : Jihong Shin (snowapril)

#if !defined(VFS_DOWNSAMPLER_H)
#define VFS_DOWNSAMPLER_H

#include <Common/pch.h>
#include <Common/EngineConfig.h>
#include <ClipmapRegion.h>
#include <VulkanFramework/CommandBuffer.h>

// TODO(snowapril) : need to move upper directory (not in renderpass directory)
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
		DownSampler& createDescriptors		(void);
		DownSampler& createPipeline			(void);
		DownSampler& createCommandPool		(const QueuePtr& computeQueue);

		inline void	cmdDownSampleOpacity	(CommandBuffer cmdBuffer, const Image* image, const ImageView* imageView, const Sampler* sampler,
											 const std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>& clipRegions,
											 uint32_t clipLevel)
		{
			cmdDownSample(cmdBuffer, image, imageView, sampler, clipRegions, clipLevel, DownSampleMode::OpacityMode);
		}
		inline void	cmdDownSampleRadiance	(CommandBuffer cmdBuffer, const Image* image, const ImageView* imageView, const Sampler* sampler,
											 const std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>& clipRegions,
											 uint32_t clipLevel)
		{
			cmdDownSample(cmdBuffer, image, imageView, sampler, clipRegions, clipLevel, DownSampleMode::RadianceMode);
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
			glm::ivec3 	prevRegionMinCorner;  	// 12
			int 		clipLevel;				// 16
			int 		clipmapResolution;		// 20
			int 		downSampleRegionSize;	// 24
		};

		// TODO(snowapril) : too many parameters in one method. need to replace them with something other class
		void cmdDownSample(CommandBuffer cmdBuffer, const Image* image, const ImageView* imageView, const Sampler* sampler,
						   const std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>& clipRegions,
						   uint32_t clipLevel, DownSampleMode mode);

	private:
		DevicePtr					_device						{ nullptr };
		DescriptorPoolPtr			_descPool					{ nullptr };
		DescriptorSetLayoutPtr		_descLayout					{ nullptr };
		DescriptorSetPtr			_descSet					{ nullptr };
		PipelineLayoutPtr			_pipelineLayout				{ nullptr };
		ComputePipelinePtr			_opacityDownSamplePipeline	{ nullptr };
		ComputePipelinePtr			_radianceDownSamplePipeline	{ nullptr };
		CommandPoolPtr				_computeCommandPool			{ nullptr };
		CommandBuffer				_computeCommandBuffer		{ nullptr };
		BufferPtr					_downSampleDescBuffer		{ nullptr };
	};
};

#endif