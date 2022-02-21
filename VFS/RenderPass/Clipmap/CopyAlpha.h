// Author : Jihong Shin (snowapril)

#if !defined(VFS_COPY_ALPHA_H)
#define VFS_COPY_ALPHA_H

#include <pch.h>
#include <Util/EngineConfig.h>
#include <RenderPass/Clipmap/ClipmapRegion.h>
#include <VulkanFramework/Commands/CommandBuffer.h>

namespace vfs
{
	class CopyAlpha : NonCopyable
	{
	public:
		explicit CopyAlpha(DevicePtr device);
				~CopyAlpha();

	public:
		CopyAlpha&	createDescriptors		(const ImageView* srcImageView,
											 const ImageView* dstImageView,
											 const Sampler* imageSampler);
		CopyAlpha&	createPipeline			(void);
		void		destroyCopyAlpha		(void);

		void cmdImageCopyAlpha(CommandBuffer cmdBuffer, const Image* dstImage, 
							   const Image* srcImage, const uint32_t clipLevel);

	private:
		struct CopyAlphaDesc
		{
			int32_t clipLevel;          //  4
			int32_t clipmapResolution;  //  8
			int32_t faceCount;			// 12
		};
				
	private:
		DevicePtr					_device					{ nullptr };
		DescriptorPoolPtr			_descPool				{ nullptr };
		DescriptorSetLayoutPtr		_descLayout				{ nullptr };
		PipelineLayoutPtr			_pipelineLayout			{ nullptr };
		ComputePipelinePtr			_pipeline				{ nullptr };
		DescriptorSetPtr			_descSet				{ nullptr };
	};
};

#endif