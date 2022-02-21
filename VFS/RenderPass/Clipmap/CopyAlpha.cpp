// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <RenderPass/Clipmap/CopyAlpha.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Descriptors/DescriptorPool.h>
#include <VulkanFramework/Descriptors/DescriptorSet.h>
#include <VulkanFramework/Descriptors/DescriptorSetLayout.h>
#include <VulkanFramework/Pipelines/ComputePipeline.h>
#include <VulkanFramework/Pipelines/PipelineLayout.h>
#include <VulkanFramework/Pipelines/PipelineConfig.h>
#include <VulkanFramework/Commands/CommandPool.h>
#include <VulkanFramework/Images/Image.h>
#include <VulkanFramework/Images/ImageView.h>
#include <VulkanFramework/Images/Sampler.h>
#include <VulkanFramework/Sync/Fence.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/Buffers/Buffer.h>

namespace vfs
{
	CopyAlpha::CopyAlpha(DevicePtr device)
		: _device(device)
	{
		// Do nothing
	}

	CopyAlpha::~CopyAlpha()
	{
		destroyCopyAlpha();
	}

	void CopyAlpha::destroyCopyAlpha(void)
	{
		_descSet.reset();
		_pipeline.reset();
		_pipelineLayout.reset();
		_descLayout.reset();
		_descPool.reset();
		_device.reset();
	}

	CopyAlpha& CopyAlpha::createDescriptors(const ImageView* srcImageView,
											const ImageView* dstImageView,
											const Sampler* imageSampler)
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
		};
		_descPool = std::make_shared<DescriptorPool>(_device, poolSizes, 1, 0);

		_descLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_descLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0);
		_descLayout->createDescriptorSetLayout(0);
		
		_descSet = std::make_shared<DescriptorSet>(_device, _descPool, _descLayout, 1);

		VkDescriptorImageInfo srcImageInfo = {};
		srcImageInfo.imageView	 = srcImageView->getImageViewHandle();
		srcImageInfo.sampler	 = imageSampler->getSamplerHandle();
		srcImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		_descSet->updateImage({ srcImageInfo }, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		
		VkDescriptorImageInfo dstImageInfo = {};
		dstImageInfo.imageView		= dstImageView->getImageViewHandle();
		dstImageInfo.sampler		= imageSampler->getSamplerHandle();
		dstImageInfo.imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
		_descSet->updateImage({ dstImageInfo }, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		return *this;
	}

	CopyAlpha& CopyAlpha::createPipeline(void)
	{
		assert(_descLayout != nullptr); // snowapril : Descriptor set layout must be initialized first

		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(_device, { _descLayout }, { { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(CopyAlphaDesc) } });

		PipelineConfig config;
		config.pipelineLayout = _pipelineLayout->getLayoutHandle();

		_pipeline = std::make_shared<ComputePipeline>();
		_pipeline->initialize(_device);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/copyAlphaImage.comp.spv", nullptr);
		_pipeline->createPipeline(&config);

		return *this;
	}

	void CopyAlpha::cmdImageCopyAlpha(CommandBuffer cmdBuffer, const Image* dstImage, 
									  const Image* srcImage, const uint32_t clipLevel)
	{
		assert(clipLevel < DEFAULT_CLIP_REGION_COUNT);
		
		CopyAlphaDesc copyAlphaDesc = {};
		copyAlphaDesc.clipLevel			= static_cast<int32_t>(clipLevel);
		copyAlphaDesc.clipmapResolution = static_cast<int32_t>(DEFAULT_VOXEL_RESOLUTION);
		copyAlphaDesc.faceCount			= static_cast<int32_t>(DEFAULT_VOXEL_FACE_COUNT);
		cmdBuffer.pushConstants(_pipelineLayout->getLayoutHandle(), VK_SHADER_STAGE_COMPUTE_BIT,
			0, sizeof(CopyAlphaDesc), &copyAlphaDesc);
		
		VkImageMemoryBarrier srcImageBarrier = srcImage->generateMemoryBarrier(
			0,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED
		);
		VkImageMemoryBarrier dstImageBarrier = dstImage->generateMemoryBarrier(
			0,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED
		);
		cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0, {}, {}, { srcImageBarrier, dstImageBarrier });
		
		cmdBuffer.bindPipeline(_pipeline);
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, _pipelineLayout->getLayoutHandle(),
			0, { _descSet }, {});
		
		const uint32_t groupCount = DEFAULT_VOXEL_RESOLUTION >> 3;
		cmdBuffer.dispatch(groupCount, groupCount, groupCount);
		
		srcImageBarrier = srcImage->generateMemoryBarrier(
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED
		);
		dstImageBarrier = dstImage->generateMemoryBarrier(
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED
		);

		cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, {}, {}, { srcImageBarrier, dstImageBarrier });
	}
};