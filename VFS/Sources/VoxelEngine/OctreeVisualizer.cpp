// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <Common/EngineConfig.h>
#include <VulkanFramework/Camera.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Buffer.h>
#include <VulkanFramework/DescriptorPool.h>
#include <VulkanFramework/DescriptorSetLayout.h>
#include <VulkanFramework/GraphicsPipeline.h>
#include <VulkanFramework/PipelineConfig.h>
#include <VulkanFramework/DescriptorSet.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/CommandPool.h>
#include <VulkanFramework/CommandBuffer.h>
#include <VulkanFramework/FrameLayout.h>
#include <VulkanFramework/Fence.h>
#include <VoxelEngine/OctreeVisualizer.h>
#include <VoxelEngine/OctreeBuilder.h>
#include <stack>
#include <tuple>

namespace vfs
{
	OctreeVisualizer::OctreeVisualizer(DevicePtr device, CameraPtr camera, std::shared_ptr<OctreeBuilder> builder, VkRenderPass renderPass)
	{
		assert(initialize(device, camera, builder, renderPass));
	}

	OctreeVisualizer::~OctreeVisualizer()
	{
		destroyOctreeVisualizer();
	}

	void OctreeVisualizer::destroyOctreeVisualizer(void)
	{
		vkDestroyPipelineLayout(_device->getDeviceHandle(), _pipelineLayout, nullptr);
	}

	bool OctreeVisualizer::initialize(DevicePtr device, CameraPtr camera, std::shared_ptr<OctreeBuilder> builder, VkRenderPass renderPass)
	{
		_device			= device;
		_camera			= camera;
		_octreeBuilder	= builder;

		if (!initializePipelineLayout())
		{
			return false;
		}

		if (!initializePipelines(renderPass))
		{
			return false;
		}
		return true;
	}

	void OctreeVisualizer::buildPointClouds(const CommandPoolPtr& cmdPool)
	{
		std::vector<glm::uvec2> nodeData;
		_octreeBuilder->readOctreeNodes(cmdPool, &nodeData);
		
		std::vector<uint32_t> brickData;
		_octreeBuilder->readOctreeBricks(cmdPool, &brickData);

		for (uint32_t i : brickData)
		{
			if (i != 0)
			{
				i = i;
			}
		}

		_pointClouds.reserve(nodeData.size());

		const uint32_t maxLevel = _octreeBuilder->getOctreeLevel();
		const uint32_t maxHalfScale = static_cast<uint32_t>((1 << maxLevel) * 0.5f);

		std::stack<std::tuple<uint32_t, uint32_t, glm::uvec3, uint32_t>> buildStack;
		buildStack.emplace(nodeData[0].x, maxLevel, glm::uvec3(maxHalfScale), 0);

		constexpr uint32_t kVisualizationTargetLevelMax = 1;
		constexpr uint32_t kVisualizationTargetLevelMin = 1;
		while (buildStack.empty() == false)
		{
			uint32_t index{ 0 }, level{ 0 }, color{ 0 };
			glm::uvec3 position;
			std::tie(index, level, position, color) = buildStack.top();
			buildStack.pop();

			if (level <= kVisualizationTargetLevelMax)
			{
				_pointClouds.emplace_back(
					((position.x & 0xffff) << 16) | (position.y & 0xffff),
					(position.z & 0xffff) << 16 | (level & 0xffff),
					color
				);
			}

			if (level <= kVisualizationTargetLevelMin) continue;

			if ((index & 0x80000000) != 0)
			{
				/*
				   0 ____ 1
					/   /|
				  2/___/3|
				  4|   | |5
				  6|___|/7
				*/
				const uint32_t baseIndex = index & 0x7fffffff;
				const  int32_t quarterScale = static_cast<int32_t>((1 << level) * 0.25f);
				for (uint32_t i = 0; i < 8; ++i)
				{
					if ((baseIndex < nodeData.size()) && ((nodeData[baseIndex + i].x & 0x80000000) != 0))
					{
						const glm::uvec3 newPos = {
							position.x + ((i & 0x1) != 0 ? -quarterScale : quarterScale),
							position.y + ((i & 0x4) != 0 ? -quarterScale : quarterScale),
							position.z + ((i & 0x2) != 0 ? -quarterScale : quarterScale)
						};
						const uint32_t newColor = brickData[nodeData[baseIndex + i].y];
						buildStack.emplace(nodeData[baseIndex + i].x, level - 1, newPos, newColor);
					}
				}
			}
		}

		_pointClouds.shrink_to_fit();

		const VmaAllocator allocator = _device->getMemoryAllocator();

		_octreeVertexBuffer = std::make_shared<Buffer>(
			allocator, 
			sizeof(glm::uvec3) * _pointClouds.size(),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		Buffer stagingBuffer(allocator, sizeof(glm::uvec3) * _pointClouds.size(),
							 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
		stagingBuffer.uploadData(_pointClouds.data(), sizeof(glm::uvec3) * _pointClouds.size());

		CommandBuffer cmdBuffer(cmdPool->allocateCommandBuffer());
		{
			cmdBuffer.beginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			VkBufferCopy copyRegion = {};
			copyRegion.size = sizeof(glm::uvec3) * _pointClouds.size();
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			cmdBuffer.copyBuffer(&stagingBuffer, _octreeVertexBuffer, { copyRegion });

			cmdBuffer.endRecord();
		}

		Fence fence(_device, 1, 0);
		cmdPool->getQueue()->submitCmdBuffer({ cmdBuffer }, &fence);

		fence.waitForAllFences(UINT64_MAX);
	}

	void OctreeVisualizer::cmdDraw(const FrameLayout* frame)
	{
		CommandBuffer cmdBuffer(frame->commandBuffer);
		cmdBuffer.bindPipeline(_avcPipeline);
		uint32_t pushValues[] = { _octreeBuilder->getOctreeLevel() };
		cmdBuffer.pushConstants(_pipelineLayout, VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(pushValues), pushValues);
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 
			{ _camera->getDescriptorSet(frame->frameIndex) }, {});
		cmdBuffer.bindVertexBuffers({ _octreeVertexBuffer }, { 0 });
		cmdBuffer.draw(static_cast<uint32_t>(_pointClouds.size()), 1, 0, 0);
	}

	bool OctreeVisualizer::initializePipelineLayout(void)
	{
		VkPushConstantRange pushConstRange = {};
		pushConstRange.offset = 0;
		pushConstRange.size = sizeof(uint32_t);
		pushConstRange.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;

		const std::vector<VkDescriptorSetLayout> descSetLayout = {
			_camera->getDescriptorSetLayout()->getLayoutHandle(),
		};
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pNext					= nullptr;
		pipelineLayoutInfo.setLayoutCount			= static_cast<uint32_t>(descSetLayout.size());
		pipelineLayoutInfo.pSetLayouts				= descSetLayout.data();
		pipelineLayoutInfo.pushConstantRangeCount	= 1;
		pipelineLayoutInfo.pPushConstantRanges		= &pushConstRange;

		if (vkCreatePipelineLayout(_device->getDeviceHandle(), &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}

	bool OctreeVisualizer::initializePipelines(VkRenderPass renderPass)
	{
		assert(_pipelineLayout != VK_NULL_HANDLE);

		PipelineConfig config;
		PipelineConfig::GetDefaultConfig(&config);

		// TODO(snowapril) : Fix hard-coded viewport & scissor
		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		viewport.width = 1280;
		viewport.height = 920;
		config.viewportInfo.pViewports = &viewport;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent.width = 1280;
		scissor.extent.height = 920;
		config.viewportInfo.pScissors = &scissor;
		config.renderPass = renderPass;
		
		config.pipelineLayout = _pipelineLayout;	
		config.inputAseemblyInfo.topology	 = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		config.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		config.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;

		VkVertexInputBindingDescription bindingDesc = {};
		bindingDesc.stride = sizeof(glm::uvec3);
		bindingDesc.binding = 0;
		bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VkVertexInputAttributeDescription attribDesc = {};
		attribDesc.offset = 0;
		attribDesc.location = 0;
		attribDesc.format = VK_FORMAT_R32G32B32_UINT;
		attribDesc.binding = 0;
		config.vertexInputInfo.vertexAttributeDescriptionCount = 1;
		config.vertexInputInfo.pVertexAttributeDescriptions = &attribDesc;
		config.vertexInputInfo.vertexBindingDescriptionCount = 1;
		config.vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;

		/*_indirectPipeline = std::make_unique<ComputePipeline>();
		_indirectPipeline->initialize(_device, { { VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/octreeAvcTransfer.comp.spv" } });
		_indirectPipeline->createPipeline(&config);*/

		_avcPipeline = std::make_shared<GraphicsPipeline>();
		_avcPipeline->initialize(_device);
		_avcPipeline->addShaderModule(VK_SHADER_STAGE_VERTEX_BIT,	"Shaders/avc.vert.spv", nullptr);
		_avcPipeline->addShaderModule(VK_SHADER_STAGE_GEOMETRY_BIT, "Shaders/avc.geom.spv", nullptr);
		_avcPipeline->addShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/avc.frag.spv", nullptr);
		_avcPipeline->createPipeline(&config);

		return true;
	}

}