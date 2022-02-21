// Author : Jihong Shin (snowapril)

#if !defined(VFS_BORDER_WRAPPER_H)
#define VFS_BORDER_WRAPPER_H

#include <pch.h>
#include <Util/EngineConfig.h>
#include <RenderPass/Clipmap/ClipmapRegion.h>
#include <VulkanFramework/Commands/CommandBuffer.h>

namespace vfs
{
	class BorderWrapper : NonCopyable
	{
	public:
		explicit BorderWrapper(DevicePtr device);
				~BorderWrapper();

	public:
		BorderWrapper&	createDescriptors		(const ImageView* opacityImageView,
												 const ImageView* radianceImageView,
												 const Sampler* clipmapSampler,
												 const CommandPoolPtr& cmdPool);
		BorderWrapper&	createPipeline			(void);
		void			destroyBorderWrapper	(void);


		inline void cmdWrappingOpacityBorder(CommandBuffer cmdBuffer, const Image* image)
		{
			cmdWrappingBorder(cmdBuffer, image, _opacityDescSet);
		}

		inline void cmdWrappingRadianceBorder(CommandBuffer cmdBuffer, const Image* image)
		{
			cmdWrappingBorder(cmdBuffer, image, _radianceDescSet);
		}

	private:
		struct BorderWrappingDesc
		{
			uint32_t clipmapResolution;	//  4
			uint32_t clipBorderWidth;	//  8
			uint32_t faceCount;			// 12
			uint32_t clipRegionCount;	// 16
		};

		void cmdWrappingBorder(CommandBuffer cmdBuffer, const Image* image, 
							   const DescriptorSetPtr& descSet);

	private:
		DevicePtr					_device						{ nullptr };
		DescriptorPoolPtr			_descPool					{ nullptr };
		DescriptorSetLayoutPtr		_descLayout					{ nullptr };
		DescriptorSetPtr			_opacityDescSet				{ nullptr };
		DescriptorSetPtr			_radianceDescSet			{ nullptr };
		PipelineLayoutPtr			_pipelineLayout				{ nullptr };
		ComputePipelinePtr			_pipeline					{ nullptr };
		BufferPtr					_borderWrappingDescBuffer	{ nullptr };
	};
};

#endif