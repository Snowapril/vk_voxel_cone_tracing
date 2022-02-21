// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <RenderPass/Clipmap/ClipmapCleaner.h>
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
	ClipmapCleaner::ClipmapCleaner(DevicePtr device)
		: _device(device)
	{
		// Do nothing
	}

	ClipmapCleaner::~ClipmapCleaner()
	{
		destroyClipmapCleaner();
	}

	void ClipmapCleaner::destroyClipmapCleaner(void)
	{
		_opacityDescSet.reset();
		_radianceDescSet.reset();
		_pipeline.reset();
		_pipelineLayout.reset();
		_descLayout.reset();
		_descPool.reset();
		_device.reset();
	}

	ClipmapCleaner& ClipmapCleaner::createDescriptors(const ImageView* opacityImageView,
													  const ImageView* radianceImageView,
													  const Sampler* clipmapSampler)
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  1},
		};
		_descPool = std::make_shared<DescriptorPool>(_device, poolSizes, 2, 0);

		_descLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  0);
		_descLayout->createDescriptorSetLayout(0);

		VkDescriptorImageInfo opacityImageInfo = {};
		opacityImageInfo.imageView	 = opacityImageView->getImageViewHandle();
		opacityImageInfo.sampler	 = clipmapSampler->getSamplerHandle();
		opacityImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		
		_opacityDescSet  = std::make_shared<DescriptorSet>(_device, _descPool, _descLayout, 1);
		_opacityDescSet->updateImage({ opacityImageInfo }, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		VkDescriptorImageInfo radianceImageInfo = {};
		radianceImageInfo.imageView		= radianceImageView->getImageViewHandle();
		radianceImageInfo.sampler		= clipmapSampler->getSamplerHandle();
		radianceImageInfo.imageLayout	= VK_IMAGE_LAYOUT_GENERAL;

		_radianceDescSet = std::make_shared<DescriptorSet>(_device, _descPool, _descLayout, 1);
		_radianceDescSet->updateImage({ radianceImageInfo }, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		return *this;
	}

	ClipmapCleaner& ClipmapCleaner::createPipeline(void)
	{
		assert(_descLayout != nullptr); // snowapril : Descriptor set layout must be initialized first

		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(_device, { _descLayout }, { { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ImageCleaningDesc) } });

		PipelineConfig config;
		config.pipelineLayout = _pipelineLayout->getLayoutHandle();

		_pipeline = std::make_shared<ComputePipeline>();
		_pipeline->initialize(_device);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/clipmapCleaning.comp.spv", nullptr);
		_pipeline->createPipeline(&config);

		return *this;
	}

	void ClipmapCleaner::cmdClearImageClipmapRegion(CommandBuffer cmdBuffer, const Image* image, glm::ivec3 regionMinCorner,
													glm::uvec3 extent, const uint32_t clipLevel, const DescriptorSetPtr& descSet)
	{
		assert(clipLevel < DEFAULT_CLIP_REGION_COUNT);

		ImageCleaningDesc imageClearDesc = {};
		imageClearDesc.regionMinCorner		= regionMinCorner;
		imageClearDesc.clipLevel			= static_cast<int32_t>(clipLevel);
		imageClearDesc.clipMaxExtent		= extent;
		imageClearDesc.clipmapResolution	= static_cast<int32_t>(DEFAULT_VOXEL_RESOLUTION);
		imageClearDesc.faceCount			= DEFAULT_VOXEL_FACE_COUNT;
		cmdBuffer.pushConstants(_pipelineLayout->getLayoutHandle(), VK_SHADER_STAGE_COMPUTE_BIT, 
								0, sizeof(ImageCleaningDesc), &imageClearDesc);

		VkImageMemoryBarrier imageBarrier = image->generateMemoryBarrier(
			0,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED
		);
		cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0, {}, {}, { imageBarrier });

		cmdBuffer.bindPipeline(_pipeline);
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, _pipelineLayout->getLayoutHandle(),
			0, { descSet }, {});

		const glm::uvec3 groupCount = glm::uvec3(glm::ceil(glm::vec3(extent) / 8.0f));
		cmdBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);

		imageBarrier = image->generateMemoryBarrier(
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED
		);
		cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, {}, {}, { imageBarrier });
	}
};