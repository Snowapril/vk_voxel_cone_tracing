// Author : Jihong Shin (rubikpril)

#include <VoxelEngine/pch.h>
#include <Common/EngineConfig.h>
#include <VulkanFramework/Camera.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Buffer.h>
#include <VulkanFramework/DescriptorPool.h>
#include <VulkanFramework/DescriptorSetLayout.h>
#include <VulkanFramework/GraphicsPipeline.h>
#include <VulkanFramework/PipelineConfig.h>
#include <VulkanFramework/DescriptorSet.h>
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

		if (!initializeBuffers())
		{
			return false;
		}

		/*if (!initializeDescriptors())
		{
			return false;
		}*/

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

	void OctreeVisualizer::buildPointClouds(VkCommandPool cmdPool)
	{
		std::vector<uint32_t> nodeData;
		_octreeBuilder->readOctreeNodes(cmdPool, &nodeData);
		
		_pointClouds.reserve(nodeData.size());

		const uint32_t maxLevel = _octreeBuilder->getOctreeLevel();
		const uint32_t maxHalfScale = static_cast<uint32_t>((1 << maxLevel) * 0.5f);

		std::stack<std::tuple<uint32_t, uint32_t, glm::uvec3>> buildStack;
		buildStack.emplace(nodeData[0], maxLevel, glm::uvec3(maxHalfScale));
		while (buildStack.empty() == false)
		{
			uint32_t index{ 0 }, level{ 0 };
			glm::uvec3 position;
			std::tie(index, level, position) = buildStack.top();
			buildStack.pop();
			if (level == 0) continue;


			if ((index & 0x80000000) != 0)
			{
				_pointClouds.emplace_back(
					((position.x & 0xffff) << 16) | (position.y & 0xffff),
					(position.z & 0xffff) << 16 | level & 0xffff
				);

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
						if ((baseIndex < nodeData.size() && nodeData[baseIndex + i] & 0x80000000) != 0)
						{
							const glm::uvec3 newPos = {
								position.x + ((i & 0x1) != 0 ? -quarterScale : quarterScale),
								position.y + ((i & 0x4) != 0 ? -quarterScale : quarterScale),
								position.z + ((i & 0x2) != 0 ? -quarterScale : quarterScale)
							};
							buildStack.emplace(nodeData[baseIndex + i], level - 1, newPos);
						}
					}
				}
			}
		}

		_pointClouds.shrink_to_fit();

		const VmaAllocator allocator = _device->getMemoryAllocator();

		_octreeVertexBuffer = std::make_shared<Buffer>(
			allocator, 
			sizeof(glm::uvec2) * _pointClouds.size(),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		BufferPtr stagingBuffer = std::make_shared<Buffer>(allocator, sizeof(glm::uvec2) * _pointClouds.size(),
														   VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
		stagingBuffer->uploadData(_pointClouds.data(), sizeof(glm::uvec2) * _pointClouds.size());

		VkCommandBufferAllocateInfo cmdBufferAllocateInfo = {};
		cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocateInfo.pNext = nullptr;
		cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufferAllocateInfo.commandPool = cmdPool;
		cmdBufferAllocateInfo.commandBufferCount = 1;

		VkCommandBuffer cmdBufferHandle{ VK_NULL_HANDLE };
		vkAllocateCommandBuffers(_device->getDeviceHandle(), &cmdBufferAllocateInfo, &cmdBufferHandle);

		CommandBuffer cmdBuffer(cmdBufferHandle);
		cmdBuffer.beginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		VkBufferCopy copyRegion = {};
		copyRegion.size		 = sizeof(glm::uvec2) * _pointClouds.size();
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		cmdBuffer.copyBuffer(stagingBuffer, _octreeVertexBuffer, { copyRegion });

		cmdBuffer.endRecord();
		
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBufferHandle;

		Fence fence(_device, 1, 0);
		vkQueueSubmit(_device->getGraphicsQueue(), 1, &submitInfo, fence.getFence(0));
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

	bool OctreeVisualizer::initializeBuffers(void)
	{
		/*const VmaAllocator allocator = _device->getMemoryAllocator();

		_octreeVertexBuffer = std::make_shared<Buffer>(
			allocator, 
			sizeof(glm::uvec2) * _octreeBuilder->getNumOctreeNodes(),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);*/

		/*_octreeStagingBuffer = std::make_shared<Buffer>(
			allocator,
			sizeof(glm::uvec2) * _octreeBuilder->getNumOctreeNodes(),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);*/
		return true;
	}

	//bool OctreeVisualizer::initializeDescriptors(void)
	//{
	//	const std::vector<VkDescriptorPoolSize> poolSizes = {
	//		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5},
	//	};
	//	_descriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 3);  // TODO(rubikpril) : 5 is this good ?

	//	_descriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
	//	_descriptorLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0);
	//	_descriptorLayout->addBinding(VK_SHADER_STAGE_COMPUTE_BIT, 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0);
	//	if (!_descriptorLayout->createDescriptorSetLayout())
	//	{
	//		return false;
	//	}

	//	_descriptorSet = std::make_unique<DescriptorSet>(_device, _descriptorPool, _descriptorLayout, 1);
	//	_descriptorSet->updateStorageBuffer(_octreeBuilder->getOctreeBuffer(), 0, 1);
	//	//_descriptorSet->updateStorageBuffer(_octreeStagingBuffer, 1, 1);
	//	return true;
	//}

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

		// TODO(rubikpril) : Fix hard-coded viewport & scissor
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
		config.rasterizationInfo.polygonMode = VK_POLYGON_MODE_LINE;

		VkVertexInputBindingDescription bindingDesc = {};
		bindingDesc.stride = sizeof(glm::uvec2);
		bindingDesc.binding = 0;
		bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VkVertexInputAttributeDescription attribDesc = {};
		attribDesc.offset = 0;
		attribDesc.location = 0;
		attribDesc.format = VK_FORMAT_R32G32_UINT;
		attribDesc.binding = 0;
		config.vertexInputInfo.vertexAttributeDescriptionCount = 1;
		config.vertexInputInfo.pVertexAttributeDescriptions = &attribDesc;
		config.vertexInputInfo.vertexBindingDescriptionCount = 1;
		config.vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;

		/*_indirectPipeline = std::make_unique<ComputePipeline>();
		_indirectPipeline->initialize(_device, { { VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/octreeAvcTransfer.comp.spv" } });
		_indirectPipeline->createPipeline(&config);*/

		_avcPipeline = std::make_shared<GraphicsPipeline>();
		_avcPipeline->initialize(_device, {
			{VK_SHADER_STAGE_VERTEX_BIT,	"Shaders/avc.vert.spv" },
			{VK_SHADER_STAGE_GEOMETRY_BIT,	"Shaders/avc.geom.spv" },
			{VK_SHADER_STAGE_FRAGMENT_BIT,	"Shaders/avc.frag.spv" },
		});
		_avcPipeline->createPipeline(&config);

		return true;
	}

}