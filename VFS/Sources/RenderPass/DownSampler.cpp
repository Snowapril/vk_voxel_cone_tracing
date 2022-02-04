// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <RenderPass/DownSampler.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/DescriptorPool.h>
#include <VulkanFramework/DescriptorSet.h>
#include <VulkanFramework/DescriptorSetLayout.h>
#include <VulkanFramework/ComputePipeline.h>
#include <VulkanFramework/PipelineLayout.h>
#include <VulkanFramework/PipelineConfig.h>
#include <VulkanFramework/CommandPool.h>
#include <VulkanFramework/Image.h>
#include <VulkanFramework/ImageView.h>
#include <VulkanFramework/Sampler.h>
#include <VulkanFramework/Fence.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/Buffer.h>

namespace vfs
{
	DownSampler::DownSampler(DevicePtr device)
		: _device(device)
	{
		// Do nothing
	}

	DownSampler::~DownSampler()
	{
		// Do nothing
	}

	DownSampler& DownSampler::createDescriptors(void)
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  1},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
		};
		_descPool = std::make_shared<DescriptorPool>(_device, poolSizes, 1, 0);

		_descLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  0);
		_descLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_descLayout->createDescriptorSetLayout(0);

		_descSet = std::make_shared<DescriptorSet>(_device, _descPool, _descLayout, 1);

		_downSampleDescBuffer = std::make_shared<Buffer>(_device->getMemoryAllocator(), sizeof(DownSampleDesc),
														 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		return *this;
	}

	DownSampler& DownSampler::createPipeline(void)
	{
		assert(_descLayout != nullptr); // snowapril : Descriptor set layout must be initialized first

		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(
			_device,
			{ _descLayout },
			{}
		);

		PipelineConfig config;
		config.pipelineLayout = _pipelineLayout->getLayoutHandle();

		_opacityDownSamplePipeline = std::make_shared<ComputePipeline>();
		_opacityDownSamplePipeline->initialize(_device);
		_opacityDownSamplePipeline->addShaderModule(VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/opacityDownSample.comp.spv", nullptr);
		_opacityDownSamplePipeline->createPipeline(&config);

		_radianceDownSamplePipeline = std::make_shared<ComputePipeline>();
		_radianceDownSamplePipeline->initialize(_device);
		_radianceDownSamplePipeline->addShaderModule(VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/radianceDownSample.comp.spv", nullptr);
		_radianceDownSamplePipeline->createPipeline(&config);

		return *this;
	}

	DownSampler& DownSampler::createCommandPool(const QueuePtr& computeQueue)
	{
		_computeCommandPool = std::make_shared<CommandPool>(_device, computeQueue, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		_computeCommandBuffer = CommandBuffer(_computeCommandPool->allocateCommandBuffer());
		return *this;
	}

	void DownSampler::cmdDownSample(CommandBuffer cmdBuffer, const Image* image, const ImageView* imageView, const Sampler* sampler,
									const std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>& clipRegions,
									uint32_t clipLevel, DownSampleMode mode)
	{
		assert(clipLevel > 0); // snowapril : clipmap level must be greater than zero for getting previous one

		DownSampleDesc downSampleDesc = {};
		downSampleDesc.prevRegionMinCorner	= clipRegions[clipLevel - 1].minCorner;
		downSampleDesc.clipLevel			= clipLevel;
		downSampleDesc.clipmapResolution	= DEFAULT_VOXEL_RESOLUTION;
		downSampleDesc.downSampleRegionSize = DEFAULT_DOWNSAMPLE_REGION_SIZE;
		_downSampleDescBuffer->uploadData(&downSampleDesc, sizeof(DownSampleDesc));
		_descSet->updateUniformBuffer({ _downSampleDescBuffer }, 1, 1);
		// TODO(snowapril) : descriptor set update must be preceded

		{
			//cmdBuffer.beginRecord(0);
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
				break;
			case DownSampleMode::RadianceMode:
				cmdBuffer.bindPipeline(_radianceDownSamplePipeline);
				break;
			default:
				assert(mode >= DownSampleMode::Last);
			}

			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageView		= imageView->getImageViewHandle();
			imageInfo.sampler		= sampler->getSamplerHandle();
			imageInfo.imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
			_descSet->updateImage({ imageInfo }, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
			cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, _pipelineLayout->getLayoutHandle(), 0, { _descSet }, {});

			const uint32_t groupCount = DEFAULT_VOXEL_RESOLUTION >> 4;
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
			cmdBuffer.endRecord();
		}

		/*Fence fence(_device, 1, 0);
		_computeCommandPool->getQueue()->submitCmdBuffer({ _computeCommandBuffer }, &fence);
		fence.waitForAllFences(UINT64_MAX);*/
	}
};