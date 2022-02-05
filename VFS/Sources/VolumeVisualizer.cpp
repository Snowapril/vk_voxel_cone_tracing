// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <Common/EngineConfig.h>
#include <Camera.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Buffer.h>
#include <VulkanFramework/DescriptorPool.h>
#include <VulkanFramework/DescriptorSetLayout.h>
#include <VulkanFramework/DescriptorSet.h>
#include <VulkanFramework/GraphicsPipeline.h>
#include <VulkanFramework/PipelineConfig.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/CommandPool.h>
#include <VulkanFramework/PipelineLayout.h>
#include <VulkanFramework/CommandBuffer.h>
#include <VulkanFramework/FrameLayout.h>
#include <VulkanFramework/Fence.h>
#include <VulkanFramework/Sampler.h>
#include <VulkanFramework/ImageView.h>
#include <VolumeVisualizer.h>

namespace vfs
{
	VolumeVisualizer::VolumeVisualizer(DevicePtr device, CameraPtr camera)
		: _device(device), _camera(camera)
	{
		// Do nothing
	}

	VolumeVisualizer& VolumeVisualizer::buildPointClouds(const CommandPoolPtr& cmdPool, uint32_t resolution)
	{
		_pointClouds.reserve(resolution * resolution * resolution);
		for (uint32_t i = 0; i < resolution; ++i)
		{
			for (uint32_t j = 0; j < resolution; ++j)
			{
				for (uint32_t k = 0; k < resolution; ++k)
				{
					_pointClouds.emplace_back(
						static_cast<float>(i),
						static_cast<float>(j),
						static_cast<float>(k)
					);
				}
			}
		}
		_pointClouds.shrink_to_fit();

		const VmaAllocator allocator = _device->getMemoryAllocator();

		_volumePointCloudBuffer = std::make_shared<Buffer>(
			allocator, 
			sizeof(glm::vec3) * _pointClouds.size(),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		Buffer stagingBuffer(allocator, sizeof(glm::vec3) * _pointClouds.size(),
							 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
		stagingBuffer.uploadData(_pointClouds.data(), sizeof(glm::vec3) * _pointClouds.size());

		CommandBuffer cmdBuffer(cmdPool->allocateCommandBuffer());
		{
			cmdBuffer.beginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			VkBufferCopy copyRegion = {};
			copyRegion.size = sizeof(glm::vec3) * _pointClouds.size();
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			cmdBuffer.copyBuffer(&stagingBuffer, _volumePointCloudBuffer, { copyRegion });

			cmdBuffer.endRecord();
		}

		Fence fence(_device, 1, 0);
		cmdPool->getQueue()->submitCmdBuffer({ cmdBuffer }, &fence);

		fence.waitForAllFences(UINT64_MAX);

		_numPoints = static_cast<uint32_t>(_pointClouds.size());
		_pointClouds.clear();
		return *this;
	}

	VolumeVisualizer& VolumeVisualizer::createDescriptorSet(ImageViewPtr tagetImageView, SamplerPtr targetSampler)
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
		};
		_descriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 1, 0);

		_descriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_GEOMETRY_BIT, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_descriptorLayout->createDescriptorSetLayout(0);

		_descriptorSet = std::make_shared<DescriptorSet>(_device, _descriptorPool, _descriptorLayout, 1);
		VkDescriptorImageInfo targetImageInfo = {};
		targetImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		targetImageInfo.sampler		= targetSampler->getSamplerHandle();
		targetImageInfo.imageView	= tagetImageView->getImageViewHandle();
		_descriptorSet->updateImage({ targetImageInfo }, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		return *this;
	}

	void VolumeVisualizer::cmdDraw(const FrameLayout* frame)
	{
		CommandBuffer cmdBuffer(frame->commandBuffer);
		cmdBuffer.bindPipeline(_avcPipeline);
		const float voxelSize = 0.125f;
		cmdBuffer.pushConstants(_pipelineLayout->getLayoutHandle(), VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(float), &voxelSize);
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 0, 
			{ _camera->getDescriptorSet(frame->frameIndex), _descriptorSet }, {});
		cmdBuffer.bindVertexBuffers({ _volumePointCloudBuffer}, { 0 });
		cmdBuffer.draw(_numPoints, 1, 0, 0);
	}

	VolumeVisualizer& VolumeVisualizer::createPipelines(VkRenderPass renderPass)
	{
		VkPushConstantRange pushConstRange = {};
		pushConstRange.offset		= 0;
		pushConstRange.size			= sizeof(float);
		pushConstRange.stageFlags	= VK_SHADER_STAGE_GEOMETRY_BIT;

		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(_device, { _camera->getDescriptorSetLayout(), _descriptorLayout }, { pushConstRange });

		PipelineConfig config;
		PipelineConfig::GetDefaultConfig(&config);

		// TODO(snowapril) : Fix hard-coded viewport & scissor
		VkViewport viewport = {};
		viewport.x			= 0;
		viewport.y			= 0;
		viewport.minDepth	= 0.0f;
		viewport.maxDepth	= 1.0f;
		viewport.width		= 1280;
		viewport.height		= 920;
		config.viewportInfo.pViewports = &viewport;

		VkRect2D scissor = {};
		scissor.offset			= { 0, 0 };
		scissor.extent.width	= 1280;
		scissor.extent.height	= 920;
		config.viewportInfo.pScissors = &scissor;
		config.renderPass = renderPass;
		
		config.pipelineLayout = _pipelineLayout->getLayoutHandle();	
		config.inputAseemblyInfo.topology	 = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		config.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		config.rasterizationInfo.cullMode	 = VK_CULL_MODE_NONE;

		VkVertexInputBindingDescription bindingDesc = {};
		bindingDesc.stride		= sizeof(glm::vec3);
		bindingDesc.binding		= 0;
		bindingDesc.inputRate	= VK_VERTEX_INPUT_RATE_VERTEX;
		VkVertexInputAttributeDescription attribDesc = {};
		attribDesc.offset	= 0;
		attribDesc.location = 0;
		attribDesc.format	= VK_FORMAT_R32G32B32_SFLOAT;
		attribDesc.binding	= 0;
		config.vertexInputInfo.vertexAttributeDescriptionCount	= 1;
		config.vertexInputInfo.pVertexAttributeDescriptions		= &attribDesc;
		config.vertexInputInfo.vertexBindingDescriptionCount	= 1;
		config.vertexInputInfo.pVertexBindingDescriptions		= &bindingDesc;

		_avcPipeline = std::make_shared<GraphicsPipeline>();
		_avcPipeline->initialize(_device);
		_avcPipeline->addShaderModule(VK_SHADER_STAGE_VERTEX_BIT,	"Shaders/avc_volume.vert.spv", nullptr);
		_avcPipeline->addShaderModule(VK_SHADER_STAGE_GEOMETRY_BIT, "Shaders/avc_volume.geom.spv", nullptr);
		_avcPipeline->addShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/avc_volume.frag.spv", nullptr);
		_avcPipeline->createPipeline(&config);

		return *this;
	}

}