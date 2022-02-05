// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Buffers/Buffer.h>
#include <VulkanFramework/Commands/CommandBuffer.h>
#include <VulkanFramework/Commands/CommandPool.h>
#include <VulkanFramework/Descriptors/DescriptorSet.h>
#include <VulkanFramework/RenderPass/RenderPass.h>
#include <VulkanFramework/Images/Image.h>
#include <VulkanFramework/QueryPool.h>
#include <VulkanFramework/RenderPass/Framebuffer.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Sync/Fence.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/Pipelines/PipelineBase.h>

namespace vfs
{
	CommandBuffer::CommandBuffer(VkCommandBuffer cmdBuffer)
		: _cmdBuffer(cmdBuffer)
	{
		// Do nothing
	}

	void CommandBuffer::beginRecord(VkCommandBufferUsageFlags usage)
	{
		VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBeginInfo.pNext = nullptr;
		cmdBufferBeginInfo.pInheritanceInfo = nullptr;
		cmdBufferBeginInfo.flags = usage;

		vkBeginCommandBuffer(_cmdBuffer, &cmdBufferBeginInfo);
	}

	void CommandBuffer::beginRenderPass(const RenderPassPtr& renderPass,
										const FramebufferPtr& framebuffer,
										const std::vector<VkClearValue>& clearValues)
	{
		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext				= nullptr;
		renderPassBeginInfo.renderPass			= renderPass->getHandle();
		renderPassBeginInfo.renderArea.offset	= { 0, 0 };
		renderPassBeginInfo.renderArea.extent	= framebuffer->getExtent();
		renderPassBeginInfo.framebuffer			= framebuffer->getFramebufferHandle();
		renderPassBeginInfo.clearValueCount		= static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.pClearValues		= clearValues.data();

		vkCmdBeginRenderPass(_cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void CommandBuffer::bindPipeline(const PipelineBasePtr& pipeline)
	{
		vkCmdBindPipeline(_cmdBuffer, pipeline->getBindPoint(), pipeline->getHandle());
	}

	void CommandBuffer::bindDescriptorSets(VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t firstSet,
										   const std::vector<DescriptorSetPtr>& descriptorSets, 
										   const std::vector<uint32_t>& dynamicOffsets)
	{
		std::vector<VkDescriptorSet> descSets(descriptorSets.size());
		for (size_t i = 0; i < descSets.size(); ++i)
		{
			descSets[i] = descriptorSets[i]->getHandle();
		}
		vkCmdBindDescriptorSets(_cmdBuffer, bindPoint, layout, firstSet,
								static_cast<uint32_t>(descSets.size()), descSets.data(),
								static_cast<uint32_t>(dynamicOffsets.size()), dynamicOffsets.data());
	}

	void CommandBuffer::copyBuffer(const Buffer* src, const BufferPtr& dst, const std::vector<VkBufferCopy>& copyRegions)
	{
		vkCmdCopyBuffer(_cmdBuffer, src->getBufferHandle(), dst->getBufferHandle(),
						static_cast<uint32_t>(copyRegions.size()), copyRegions.data());
	}

	void CommandBuffer::copyBufferToImage(const Buffer* src, const ImagePtr& dst,
								  const std::vector<VkBufferImageCopy>& copyRegions)
	{
		vkCmdCopyBufferToImage(_cmdBuffer, 
							   src->getBufferHandle(), dst->getImageHandle(), 
							   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							   static_cast<uint32_t>(copyRegions.size()), copyRegions.data());
	}

	void CommandBuffer::bindVertexBuffers(const std::vector<BufferPtr>& buffers, const std::vector<VkDeviceSize>& offsets)
	{
		std::vector<VkBuffer> bufferHandles(buffers.size());
		for (size_t i = 0; i < bufferHandles.size(); ++i)
		{
			bufferHandles[i] = buffers[i]->getBufferHandle();
		}
		vkCmdBindVertexBuffers(_cmdBuffer, 0, static_cast<uint32_t>(bufferHandles.size()), bufferHandles.data(), offsets.data());
	}

	void CommandBuffer::bindIndexBuffer(const BufferPtr& buffer, const VkDeviceSize offset, VkIndexType indexType)
	{
		vkCmdBindIndexBuffer(_cmdBuffer, buffer->getBufferHandle(), offset, indexType);
	}

	void CommandBuffer::writeTimeStamp(VkPipelineStageFlagBits stageFlag, const QueryPoolPtr& queryPool, uint32_t queryIndex)
	{
		vkCmdWriteTimestamp(_cmdBuffer, stageFlag, queryPool->getHandle(), queryIndex);
	}

	void CommandBuffer::resetQueryPool(const QueryPoolPtr& queryPool, uint32_t numQuery)
	{
		vkCmdResetQueryPool(_cmdBuffer, queryPool->getHandle(), 0, numQuery);
	}

	void CommandBuffer::dispatchIndirect(const BufferPtr& buffer, const VkDeviceSize offset)
	{
		vkCmdDispatchIndirect(_cmdBuffer, buffer->getBufferHandle(), offset);
	}

	void CommandBuffer::blitImage(const ImagePtr& srcImage, VkImageLayout srcImageLayout,
								  const ImagePtr& dstImage, VkImageLayout dstImageLayout,
								  const std::vector<VkImageBlit>& blits, VkFilter filter)
	{
		vkCmdBlitImage(_cmdBuffer, 
			srcImage->getImageHandle(), srcImageLayout,
			dstImage->getImageHandle(), dstImageLayout, 
			static_cast<uint32_t>(blits.size()), blits.data(), filter
		);
	}
}