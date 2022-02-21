// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <RenderPass/Clipmap/DownSampler.h>
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
	DownSampler::DownSampler(DevicePtr device)
		: _device(device)
	{
		// Do nothing
	}

	DownSampler::~DownSampler()
	{
		destroyDownSampler();
	}

	void DownSampler::destroyDownSampler(void)
	{
		for (uint32_t i = 0; i < DEFAULT_CLIP_REGION_COUNT - 1; ++i)
		{
			_opacityDownSampleDescSet[i].reset();
			_radianceDownSampleDescSet[i].reset();
			_downSampleDescBuffer[i].reset();
		}
		_opacityDownSamplePipeline.reset();
		_radianceDownSamplePipeline.reset();
		_pipelineLayout.reset();
		_descLayout.reset();
		_descPool.reset();
		_device.reset();
	}

	DownSampler& DownSampler::createDescriptors(const ImageView* opacityImageView, 
												const ImageView* radianceImageView, 
												const Sampler* clipmapSampler)
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  1},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
		};
		_descPool = std::make_shared<DescriptorPool>(_device, poolSizes, 2 * (DEFAULT_CLIP_REGION_COUNT - 1), 0);

		_descLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  0);
		_descLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_descLayout->createDescriptorSetLayout(0);

		VkDescriptorImageInfo opacityImageInfo = {};
		opacityImageInfo.imageView		= opacityImageView->getImageViewHandle();
		opacityImageInfo.sampler		= clipmapSampler->getSamplerHandle();
		opacityImageInfo.imageLayout	= VK_IMAGE_LAYOUT_GENERAL;

		VkDescriptorImageInfo radianceImageInfo = {};
		radianceImageInfo.imageView		= radianceImageView->getImageViewHandle();
		radianceImageInfo.sampler		= clipmapSampler->getSamplerHandle();
		radianceImageInfo.imageLayout	= VK_IMAGE_LAYOUT_GENERAL;

		for (uint32_t i = 0; i < DEFAULT_CLIP_REGION_COUNT - 1; ++i)
		{
			_downSampleDescBuffer[i] = std::make_shared<Buffer>(_device->getMemoryAllocator(), sizeof(DownSampleDesc),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

			_opacityDownSampleDescSet[i]  = std::make_shared<DescriptorSet>(_device, _descPool, _descLayout, 1);
			_radianceDownSampleDescSet[i] = std::make_shared<DescriptorSet>(_device, _descPool, _descLayout, 1);

			_opacityDownSampleDescSet[i]->updateUniformBuffer({ _downSampleDescBuffer[i] }, 1, 1);
			_radianceDownSampleDescSet[i]->updateUniformBuffer({ _downSampleDescBuffer[i] }, 1, 1);

			_opacityDownSampleDescSet[i]->updateImage({ opacityImageInfo }, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
			_radianceDownSampleDescSet[i]->updateImage({ radianceImageInfo }, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		}
		
		return *this;
	}

	DownSampler& DownSampler::createPipeline(void)
	{
		assert(_descLayout != nullptr); // snowapril : Descriptor set layout must be initialized first

		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(_device, { _descLayout }, {});

		PipelineConfig config;
		config.pipelineLayout = _pipelineLayout->getLayoutHandle();

		_opacityDownSamplePipeline = std::make_shared<ComputePipeline>();
		_opacityDownSamplePipeline->initialize(_device);
		_opacityDownSamplePipeline->attachShaderModule(VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/opacityDownSample.comp.spv", nullptr);
		_opacityDownSamplePipeline->createPipeline(&config);

		_radianceDownSamplePipeline = std::make_shared<ComputePipeline>();
		_radianceDownSamplePipeline->initialize(_device);
		_radianceDownSamplePipeline->attachShaderModule(VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/radianceDownSample.comp.spv", nullptr);
		_radianceDownSamplePipeline->createPipeline(&config);

		return *this;
	}

	void DownSampler::cmdDownSample(CommandBuffer cmdBuffer, const Image* image,
									const std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>& clipRegions,
									uint32_t clipLevel, DownSampleMode mode)
	{
		assert(clipLevel > 0); // snowapril : clipmap level must be greater than zero for getting previous one

		DownSampleDesc downSampleDesc = {};
		downSampleDesc.prevRegionMinCorner	= clipRegions[clipLevel - 1].minCorner;
		downSampleDesc.clipLevel			= clipLevel;
		downSampleDesc.clipmapResolution	= DEFAULT_VOXEL_RESOLUTION;
		downSampleDesc.downSampleRegionSize = DEFAULT_DOWNSAMPLE_REGION_SIZE;
		_downSampleDescBuffer[clipLevel - 1]->uploadData(&downSampleDesc, sizeof(DownSampleDesc));

		VkImageMemoryBarrier imageBarrier = image->generateMemoryBarrier(
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED
		);
		cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0, {}, {}, { imageBarrier });

		switch (mode)
		{
		case DownSampleMode::OpacityMode:
			cmdBuffer.bindPipeline(_opacityDownSamplePipeline);
			_opacityDownSampleDescSet[clipLevel - 1]->updateUniformBuffer({ _downSampleDescBuffer[clipLevel - 1] }, 1, 1);
			cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, _pipelineLayout->getLayoutHandle(),
										 0, { _opacityDownSampleDescSet[clipLevel - 1] }, {});
			break;
		case DownSampleMode::RadianceMode:
			cmdBuffer.bindPipeline(_radianceDownSamplePipeline);
			_radianceDownSampleDescSet[clipLevel - 1]->updateUniformBuffer({ _downSampleDescBuffer[clipLevel - 1] }, 1, 1);
			cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, _pipelineLayout->getLayoutHandle(),
										 0, { _radianceDownSampleDescSet[clipLevel - 1] }, {});
			break;
		default:
			assert(mode >= DownSampleMode::Last);
		}

		constexpr uint32_t groupCount = DEFAULT_VOXEL_RESOLUTION >> 4;
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