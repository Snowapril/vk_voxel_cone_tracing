// Author : Jihong Shin (rubikpril)

#include <VoxelEngine/pch.h>
#include <Common/EngineConfig.h>
#include <VulkanFramework/Buffer.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/PipelineConfig.h>
#include <VulkanFramework/Fence.h>
#include <VulkanFramework/Scene.h>
#include <VulkanFramework/DescriptorPool.h>
#include <VulkanFramework/DescriptorSetLayout.h>
#include <VoxelEngine/Voxelizer.h>
#include <VoxelEngine/Counter.h>
#include <iostream>

namespace vfs
{
	SparseVoxelizer::SparseVoxelizer(DevicePtr device, std::shared_ptr<Scene> scene, const uint32_t octreeLevel)
	{
		assert(initialize(device, scene, octreeLevel));
	}

	SparseVoxelizer::~SparseVoxelizer()
	{
		destroyVoxelizer();
	}

	void SparseVoxelizer::destroyVoxelizer(void)
	{
		vkDestroyPipelineLayout(_device->getDeviceHandle(), _pipelineLayout, nullptr);
		_framebuffer.reset();
		vkDestroyRenderPass(_device->getDeviceHandle(), _renderPass, nullptr);
		_counter.reset();
		_device.reset();
	}

	bool SparseVoxelizer::initialize(DevicePtr device, std::shared_ptr<Scene> scene, const uint32_t octreeLevel)
	{
		assert(octreeLevel <= MAX_OCTREE_LEVEL); // rubikpril : octree level must be smaller than MAX_OCTREE_LEVEL
		_device			 = device;
		_scene			 = scene;
		_voxelResolution = 1 << octreeLevel;
		_level			 = octreeLevel;

		_counter = std::make_unique<Counter>();
		if (!_counter->initialize(_device))
		{
			std::cerr << "[VoxelEngine] Failed to create counter\n";
			return false;
		}

		if (!initializeRenderPass())
		{
			std::cerr << "[VoxelEngine] Failed to create render pass for voxelization pipeline\n";
			return false;
		}

		if (!initializeDescriptors())
		{
			std::cerr << "[VoxelEngine] Failed to create descriptors\n";
			return false;
		}

		if (!initializePipelineLayout())
		{
			std::cerr << "[VoxelEngine] Failed to create pipeline layout\n";
			return false;
		}

		if (!initializePipeline(_renderPass))
		{
			std::cerr << "[VoxelEngine] Failed to create voxelization pipeline\n";
			return false;
		}

		return true;
	}

	bool SparseVoxelizer::initializeRenderPass(void)
	{
		VkSubpassDescription subPassDesc = {};
		subPassDesc.colorAttachmentCount = 0;
		subPassDesc.pColorAttachments = nullptr;
		subPassDesc.pDepthStencilAttachment = nullptr;
		subPassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pNext = nullptr;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subPassDesc;
		if (vkCreateRenderPass(_device->getDeviceHandle(), &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS)
		{
			return false;
		}

		// TODO(rubikpril) : Pop framebuffer init to method
		_framebuffer = std::make_unique<Framebuffer>();
		if (!_framebuffer->initialize(_device, {}, _renderPass, { _voxelResolution, _voxelResolution }))
		{
			return false;
		}
		return true;
	}

	bool SparseVoxelizer::initializeDescriptors(void)
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2},
		};
		_descriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 2);  // TODO(rubikpril) : 2 is this good ?

		_descriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);;
		if (!_descriptorLayout->createDescriptorSetLayout())
		{
			return false;
		}

		_descriptorSet = std::make_unique<DescriptorSet>(_device, _descriptorPool, _descriptorLayout, 1);

		_descriptorSet->updateStorageBuffer(_counter->getBuffer(), 0, 1);
		return true;
	}

	bool SparseVoxelizer::initializePipelineLayout(void)
	{
		VkPushConstantRange pushConstRange = {};
		pushConstRange.offset		= 0;
		pushConstRange.size			= sizeof(uint32_t) * 2;
		pushConstRange.stageFlags	= VK_SHADER_STAGE_FRAGMENT_BIT;

		const VkDescriptorSetLayout descSetLayout = _descriptorLayout->getLayoutHandle();
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pNext = nullptr;
		pipelineLayoutInfo.setLayoutCount	= 1;
		pipelineLayoutInfo.pSetLayouts		= &descSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount	= 1;
		pipelineLayoutInfo.pPushConstantRanges		= &pushConstRange;

		if (vkCreatePipelineLayout(_device->getDeviceHandle(), &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}

	bool SparseVoxelizer::initializePipeline(VkRenderPass renderPass)
	{
		assert(_pipelineLayout != VK_NULL_HANDLE); // rubikpril : pipeline layout must be initialized first

		PipelineConfig config;
		PipelineConfig::GetDefaultConfig(&config);

		VkVertexInputBindingDescription bindingDesc = _scene->getVertexInputBindingDesc(0);
		std::vector<VkVertexInputAttributeDescription> attribDescs = _scene->getVertexInputAttribDesc(0);
		config.vertexInputInfo.vertexAttributeDescriptionCount	= static_cast<uint32_t>(attribDescs.size());
		config.vertexInputInfo.pVertexAttributeDescriptions		= attribDescs.data();
		config.vertexInputInfo.vertexBindingDescriptionCount	= 1;
		config.vertexInputInfo.pVertexBindingDescriptions		= &bindingDesc;
		config.inputAseemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		
		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		viewport.width  = static_cast<float>(_voxelResolution);
		viewport.height = static_cast<float>(_voxelResolution);
		config.viewportInfo.pViewports = &viewport;

		VkRect2D scissor = {};
		scissor.offset			= { 0, 0 };
		scissor.extent.width	= _voxelResolution;
		scissor.extent.height	= _voxelResolution;
		config.viewportInfo.pScissors = &scissor;

		config.renderPass		= renderPass;
		config.pipelineLayout	= _pipelineLayout;

		std::vector<std::pair<VkShaderStageFlagBits, const char*>> shaderStages{
			{VK_SHADER_STAGE_VERTEX_BIT,	"Shaders/voxelizer.vert.spv" },
			{VK_SHADER_STAGE_GEOMETRY_BIT,	"Shaders/voxelizer.geom.spv" },
			{VK_SHADER_STAGE_FRAGMENT_BIT,	"Shaders/voxelizer.frag.spv" },
		};
		_pipeline = std::make_unique<GraphicsPipeline>(_device, std::move(shaderStages));
		_pipeline->createPipeline(&config);
		return true;
	}

	void SparseVoxelizer::preVoxelize(VkCommandPool cmdPool)
	{
		_counter->resetCounter(cmdPool);

		VkCommandBuffer cmdBuffer;
		VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
		cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocInfo.pNext = nullptr;
		cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufferAllocInfo.commandBufferCount = 1;
		cmdBufferAllocInfo.commandPool = cmdPool;
		vkAllocateCommandBuffers(_device->getDeviceHandle(), &cmdBufferAllocInfo, &cmdBuffer);

		VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBeginInfo.pNext = nullptr;
		cmdBufferBeginInfo.pInheritanceInfo = nullptr;
		cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo);

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = _renderPass;
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = { _voxelResolution, _voxelResolution };
		renderPassBeginInfo.framebuffer = _framebuffer->getFramebufferHandle();
		renderPassBeginInfo.clearValueCount = 0;
		renderPassBeginInfo.pClearValues = nullptr;

		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport;
		viewport.x			 = 0;
		viewport.y			 = 0;
		viewport.width		= static_cast<float>(_voxelResolution);
		viewport.height		= static_cast<float>(_voxelResolution);
		viewport.minDepth	= 0.0f;
		viewport.maxDepth	= 1.0f;

		VkRect2D scissor;
		scissor.offset = { 0, 0 };
		scissor.extent = { _voxelResolution, _voxelResolution };

		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		const VkDescriptorSet descSet = _descriptorSet->getHandle();
		_pipeline->bindPipeline(cmdBuffer);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &descSet, 0, nullptr);
		uint32_t pushValues[] = { _voxelResolution, 1 };
		vkCmdPushConstants(cmdBuffer, _pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t) * 2, pushValues);
		_scene->cmdDraw(cmdBuffer);

		vkCmdEndRenderPass(cmdBuffer);
		vkEndCommandBuffer(cmdBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.pCommandBuffers = &cmdBuffer;
		submitInfo.commandBufferCount = 1;
		
		Fence fence(_device, 1, 0);
		vkQueueSubmit(_device->getGraphicsQueue(), 1, &submitInfo, fence.getFence(0));

		fence.waitForAllFences(UINT64_MAX);
		_voxelFragmentCount = _counter->readCounterValue(cmdPool);
		_counter->resetCounter(cmdPool);

		// TODO(rubikpril) : fragment list update
		_fragmentList = std::make_shared<Buffer>();
		_fragmentList->initialize(_device->getMemoryAllocator(), sizeof(uint32_t) * 2 * _voxelFragmentCount,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
		
		VkDescriptorBufferInfo fragmentBufferInfo = {};
		fragmentBufferInfo.buffer = _fragmentList->getBufferHandle();
		fragmentBufferInfo.offset = 0;
		fragmentBufferInfo.range = _fragmentList->getTotalSize();
		
		VkWriteDescriptorSet writeSet = {};
		writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSet.pNext = nullptr;
		writeSet.dstBinding = 1;
		writeSet.dstSet = _descriptorSet->getHandle();
		writeSet.descriptorCount = 1;
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writeSet.pBufferInfo = &fragmentBufferInfo;
		
		vkUpdateDescriptorSets(_device->getDeviceHandle(), 1, &writeSet, 0, nullptr);
		std::clog << "[VoxelEngine] Voxel fragment list : " << (_fragmentList->getTotalSize() / 1000) << " KB\n";
	}

	void SparseVoxelizer::cmdVoxelize(VkCommandBuffer cmdBuffer)
	{
		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = _renderPass;
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = { _voxelResolution, _voxelResolution };
		renderPassBeginInfo.framebuffer = _framebuffer->getFramebufferHandle();
		renderPassBeginInfo.clearValueCount = 0;
		renderPassBeginInfo.pClearValues = nullptr;

		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport;
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = static_cast<float>(_voxelResolution);
		viewport.height = static_cast<float>(_voxelResolution);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor;
		scissor.offset = { 0, 0 };
		scissor.extent = { _voxelResolution, _voxelResolution };

		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		const VkDescriptorSet descSet = _descriptorSet->getHandle();
		_pipeline->bindPipeline(cmdBuffer);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &descSet, 0, nullptr);
		uint32_t pushValues[] = { _voxelResolution, 0 };
		vkCmdPushConstants(cmdBuffer, _pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t) * 2, pushValues);
		_scene->cmdDraw(cmdBuffer);

		vkCmdEndRenderPass(cmdBuffer);
	}

	void SparseVoxelizer::readVoxelFragments(VkCommandPool commandPool, OUT std::vector<glm::uvec3>* readData)
	{
		Buffer stagingBuffer;
		stagingBuffer.initialize(_device->getMemoryAllocator(), sizeof(uint32_t) * 2 * _voxelFragmentCount,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		VkCommandBufferAllocateInfo cmdBufferAllocateInfo = {};
		cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocateInfo.pNext = nullptr;
		cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufferAllocateInfo.commandPool = commandPool;
		cmdBufferAllocateInfo.commandBufferCount = 1;

		VkCommandBuffer cmdBuffer{ VK_NULL_HANDLE };
		if (vkAllocateCommandBuffers(_device->getDeviceHandle(), &cmdBufferAllocateInfo, &cmdBuffer) != VK_SUCCESS)
		{
			std::cerr << "[VoxelEngine] Failed to allocate command buffer for read counter value\n";
		}

		VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBeginInfo.pNext = nullptr;
		cmdBufferBeginInfo.pInheritanceInfo = nullptr;
		cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo) != VK_SUCCESS)
		{
			std::cerr << "[VoxelEngine] Failed to begin command buffer for read counter value\n";
		}

		VkBufferCopy bufferCopyInfo = {};
		bufferCopyInfo.size = sizeof(uint32_t) * 2 * _voxelFragmentCount;
		bufferCopyInfo.srcOffset = 0;
		bufferCopyInfo.dstOffset = 0;
		vkCmdCopyBuffer(cmdBuffer, _fragmentList->getBufferHandle(), stagingBuffer.getBufferHandle(), 1, &bufferCopyInfo);

		if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS)
		{
			std::cerr << "[VoxelEngine] Failed to end command buffer for read counter value\n";
		}

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;

		Fence fence(_device, 1, 0);
		if (vkQueueSubmit(_device->getGraphicsQueue(), 1, &submitInfo, fence.getFence(0)) != VK_SUCCESS)
		{
			std::cerr << "[VoxelEngine] Failed to submit command buffer to "
				"graphics queue for read counter value\n";
		}

		if (!fence.waitForAllFences(UINT64_MAX))
		{
			std::cerr << "[VoxelEngine] Failed to wait fence read counter value\n";
		}

		vkFreeCommandBuffers(_device->getDeviceHandle(), commandPool, 1, &cmdBuffer);

		std::vector<glm::uvec2> downloadedData(_voxelFragmentCount);
		stagingBuffer.downloadData(downloadedData.data(), sizeof(glm::uvec2) * _voxelFragmentCount);

		// for testing
		readData->reserve(_voxelFragmentCount);
		for (auto& v : downloadedData)
		{
			uint32_t x = v.x & 0xfff;
			uint32_t y = (v.x >> 12) & 0xfff;
			uint32_t z = (v.x >> 24) & 0xff | ((((v.y >> 28) & 0xf) << 8) & 0xf00);
			readData->emplace_back(x, y, z);
		}
	}
}