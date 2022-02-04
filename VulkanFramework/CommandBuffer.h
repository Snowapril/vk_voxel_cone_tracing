// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_COMMAND_BUFFER_H)
#define VULKAN_FRAMEWORK_COMMAND_BUFFER_H

#include <VulkanFramework/pch.h>

namespace vfs
{
	class CommandBuffer
	{
	public:
		explicit CommandBuffer(VkCommandBuffer cmdBuffer);
				 CommandBuffer(const CommandBuffer&) = default;
		CommandBuffer& operator=(const CommandBuffer&) = default;

	public:
		inline VkCommandBuffer getHandle(void) const
		{
			return _cmdBuffer;
		}

		void beginRecord		(VkCommandBufferUsageFlags usage);
		void beginRenderPass	(const RenderPassPtr& renderPass, 
								 const FramebufferPtr& framebuffer,
								 const std::vector<VkClearValue>& clearValues);
		void bindPipeline		(const PipelineBasePtr& pipeline);
		void bindDescriptorSets	(VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t firstSet,
								 const std::vector<DescriptorSetPtr>& descriptorSets, 
								 const std::vector<uint32_t>& dynamicOffsets);
		void copyBuffer			(const Buffer* src, const BufferPtr& dst, 
								 const std::vector<VkBufferCopy>& copyRegions);
		void copyBufferToImage	(const Buffer* src, const ImagePtr& dst,
								 const std::vector<VkBufferImageCopy>& copyRegions);
		void bindVertexBuffers	(const std::vector<BufferPtr>& buffers, const std::vector<VkDeviceSize>& offsets);
		void bindIndexBuffer	(const BufferPtr& buffer, const VkDeviceSize offset, VkIndexType indexType);
		void writeTimeStamp		(VkPipelineStageFlagBits stageFlag, const QueryPoolPtr& queryPool, uint32_t queryIndex);
		void resetQueryPool		(const QueryPoolPtr& queryPool, uint32_t numQuery);
		void dispatchIndirect	(const BufferPtr& buffer, const VkDeviceSize offset);
		void blitImage(const ImagePtr& srcImage, VkImageLayout srcImageLayout,
					   const ImagePtr& dstImage, VkImageLayout dstImageLayout,
					   const std::vector<VkImageBlit>& blits, VkFilter filter);

		inline void endRenderPass(void)
		{
			vkCmdEndRenderPass(_cmdBuffer);
		}
		inline void endRecord(void)
		{
			vkEndCommandBuffer(_cmdBuffer);
		}
		inline void setViewport(const std::vector<VkViewport>& viewport)
		{
			vkCmdSetViewport(_cmdBuffer, 0, static_cast<uint32_t>(viewport.size()), viewport.data());
		}
		inline void setScissor(const std::vector<VkRect2D>& scissors)
		{
			vkCmdSetScissor(_cmdBuffer, 0, static_cast<uint32_t>(scissors.size()), scissors.data());
		}
		inline void pushConstants(VkPipelineLayout layout, VkShaderStageFlags stage, 
								  uint32_t offset, uint32_t size, const void* value)
		{
			vkCmdPushConstants(_cmdBuffer, layout, stage, offset, size, value);
		}
		inline void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
		{
			vkCmdDraw(_cmdBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
		}
		inline void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
								uint32_t vertexOffset, uint32_t firstInstance)
		{
			vkCmdDrawIndexed(_cmdBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
		}
		inline void dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
		{
			vkCmdDispatch(_cmdBuffer, groupX, groupY, groupZ);
		}
		inline void pipelineBarrier(VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkDependencyFlags dependency,
									const std::vector<VkMemoryBarrier>&			memoryBarrier,
									const std::vector<VkBufferMemoryBarrier>&	bufferMemoryBarrier,
									const std::vector<VkImageMemoryBarrier>&	imageMemoryBarrier)
		{
			vkCmdPipelineBarrier(_cmdBuffer, srcStage, dstStage, dependency,
				static_cast<uint32_t>(memoryBarrier.size()), memoryBarrier.data(),
				static_cast<uint32_t>(bufferMemoryBarrier.size()), bufferMemoryBarrier.data(),
				static_cast<uint32_t>(imageMemoryBarrier.size()), imageMemoryBarrier.data()
			);
		}
		
	private:
		VkCommandBuffer _cmdBuffer { VK_NULL_HANDLE };
	};
}

#endif