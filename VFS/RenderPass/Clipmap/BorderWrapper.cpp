// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <RenderPass/Clipmap/BorderWrapper.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Commands/CommandBuffer.h>
#include <VulkanFramework/Commands/CommandPool.h>
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
	BorderWrapper::BorderWrapper(DevicePtr device)
		: _device(device)
	{
		// Do nothing
	}

	BorderWrapper::~BorderWrapper()
	{
		destroyBorderWrapper();
	}

	void BorderWrapper::destroyBorderWrapper(void)
	{
		_borderWrappingDescBuffer.reset();
		_pipeline.reset();
		_pipelineLayout.reset();
		_opacityDescSet.reset();
		_radianceDescSet.reset();
		_descLayout.reset();
		_descPool.reset();
		_device.reset();
	}

	BorderWrapper& BorderWrapper::createDescriptors(const ImageView* opacityImageView,
													const ImageView* radianceImageView,
													const Sampler* clipmapSampler,
													const CommandPoolPtr& cmdPool)
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  1},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
		};
		_descPool = std::make_shared<DescriptorPool>(_device, poolSizes, 2, 0);

		_descLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  0);
		_descLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_descLayout->createDescriptorSetLayout(0);

		_opacityDescSet  = std::make_shared<DescriptorSet>(_device, _descPool, _descLayout, 1);
		_radianceDescSet = std::make_shared<DescriptorSet>(_device, _descPool, _descLayout, 1);

		_borderWrappingDescBuffer = std::make_shared<Buffer>(_device->getMemoryAllocator(), sizeof(BorderWrappingDesc),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
		
		BorderWrappingDesc borderWrappingDesc = {};
		borderWrappingDesc.clipmapResolution = DEFAULT_VOXEL_RESOLUTION;
		borderWrappingDesc.clipBorderWidth = DEFAULT_VOXEL_BORDER;
		borderWrappingDesc.faceCount = DEFAULT_VOXEL_FACE_COUNT;
		borderWrappingDesc.clipRegionCount = DEFAULT_CLIP_REGION_COUNT;

		Buffer stagingBuffer(_device->getMemoryAllocator(), sizeof(BorderWrappingDesc),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
		stagingBuffer.uploadData(&borderWrappingDesc, sizeof(BorderWrappingDesc));

		cmdPool->submitOnce([&](CommandBuffer cmdBuffer) {
			VkBufferCopy copyRegion = {};
			copyRegion.size = sizeof(BorderWrappingDesc);
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			cmdBuffer.copyBuffer(&stagingBuffer, _borderWrappingDescBuffer, { copyRegion });
		});

		_opacityDescSet->updateUniformBuffer({ _borderWrappingDescBuffer }, 1, 1);
		_radianceDescSet->updateUniformBuffer({ _borderWrappingDescBuffer }, 1, 1);

		VkDescriptorImageInfo opacityImageInfo = {};
		opacityImageInfo.imageView	 = opacityImageView->getImageViewHandle();
		opacityImageInfo.sampler	 = clipmapSampler->getSamplerHandle();
		opacityImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		_opacityDescSet->updateImage({ opacityImageInfo }, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		VkDescriptorImageInfo radianceImageInfo = {};
		radianceImageInfo.imageView		= radianceImageView->getImageViewHandle();
		radianceImageInfo.sampler		= clipmapSampler->getSamplerHandle();
		radianceImageInfo.imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
		_radianceDescSet->updateImage({ radianceImageInfo }, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		return *this;
	}

	BorderWrapper& BorderWrapper::createPipeline(void)
	{
		assert(_descLayout != nullptr); // snowapril : Descriptor set layout must be initialized first

		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(_device, { _descLayout }, {});

		PipelineConfig config;
		config.pipelineLayout = _pipelineLayout->getLayoutHandle();

		_pipeline = std::make_shared<ComputePipeline>();
		_pipeline->initialize(_device);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/borderWrapping.comp.spv", nullptr);
		_pipeline->createPipeline(&config);

		return *this;
	}

	void BorderWrapper::cmdWrappingBorder(CommandBuffer cmdBuffer, const Image* image, const DescriptorSetPtr& descSet)
	{
		VkImageMemoryBarrier imageBarrier = image->generateMemoryBarrier(
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED
		);
		cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0, {}, {}, { imageBarrier });

		cmdBuffer.bindPipeline(_pipeline);
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, _pipelineLayout->getLayoutHandle(),
			0, { descSet }, {});

		constexpr uint32_t groupCount = (DEFAULT_VOXEL_RESOLUTION + DEFAULT_VOXEL_BORDER) >> 3;
		cmdBuffer.dispatch(groupCount, groupCount, groupCount);

		imageBarrier = image->generateMemoryBarrier(
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED
		);
		cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0, {}, {}, { imageBarrier });
	}
};