// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <Common/Logger.h>
#include <Util/EngineConfig.h>
#include <Renderer.h>
#include <SwapChain.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Images/Image.h>
#include <VulkanFramework/Images/ImageView.h>
#include <VulkanFramework/RenderPass/RenderPass.h>
#include <VulkanFramework/RenderPass/Framebuffer.h>
#include <VulkanFramework/Commands/CommandPool.h>
#include <VulkanFramework/Utils.h>
#include <VulkanFramework/Window.h>
#include <thread>
#include <chrono>

namespace vfs
{
	Renderer::Renderer(vfs::DevicePtr device,
					   vfs::CommandPoolPtr mainCmdPool,
					   std::unique_ptr<SwapChain>&& swapChain)
	{
		assert(initialize(device, mainCmdPool, std::move(swapChain)));
	}
	
	Renderer::~Renderer()
	{
		destroyRenderer();
	}
	
	void Renderer::destroyRenderer(void)
	{
		_mainCmdPool->freeCommandBuffers(_commandBuffers);
		_commandBuffers.clear();
		_swapChain.reset();
		if (_surface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(_device->getVulkanInstance(), _surface, nullptr);
			_surface = VK_NULL_HANDLE;
		}
		_device.reset();
	}
	
	bool Renderer::initialize(vfs::DevicePtr device,
							  vfs::CommandPoolPtr mainCmdPool,
							  std::unique_ptr<SwapChain>&& swapChain)
	{
		_device			= device;
		_mainCmdPool	= mainCmdPool;
		_swapChain		= std::move(swapChain);
		_commandBuffers = _mainCmdPool->allocateMultipleCommandBuffer(DEFAULT_NUM_FRAMES);
		_surface		= _swapChain->getSurfaceHandle();

		return true;
	}
	
	bool Renderer::recreateSwapChain(void)
	{
		vkDeviceWaitIdle(_device->getDeviceHandle());
		std::unique_ptr<SwapChain>&& tempSwapChain = std::move(_swapChain);
		_swapChain = std::make_unique<SwapChain>(std::move(tempSwapChain));
		return true;
	}
	
	VkCommandBuffer Renderer::beginFrame(void)
	{
		assert(!_isFrameStarted);
	
		VkResult result = _swapChain->acquireNextImage(&_currentImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain();
			return VK_NULL_HANDLE;
		}
	
		VkCommandBuffer& currentCommandBuffer = getCurrentCommandBuffer();
		VkCommandBufferBeginInfo commandBufferBeginInfo = {};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.pNext = nullptr;
		commandBufferBeginInfo.pInheritanceInfo = nullptr;
		commandBufferBeginInfo.flags = 0;
	
		if (vkBeginCommandBuffer(currentCommandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
		{
			VFS_ERROR << "Failed to begin command buffer";
			return VK_NULL_HANDLE;
		}
	
		_isFrameStarted = true;
		return currentCommandBuffer;
	}
	
	void Renderer::endFrame(void)
	{
		assert(_isFrameStarted);
	
		VkCommandBuffer& currentCommandBuffer = getCurrentCommandBuffer();
		vkEndCommandBuffer(currentCommandBuffer);

		VkResult result = _swapChain->submitCommandBuffer(&currentCommandBuffer, &_currentImageIndex,
														  _waitSemaphores, _signalSemaphores);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _swapChain->getWindowPtr()->wasWindowResized())
		{
			_swapChain->getWindowPtr()->setWindowResizedFlag(false);
			recreateSwapChain();
		}
		
		_isFrameStarted = false;
		_currentFrameIndex = (_currentFrameIndex + 1) % DEFAULT_NUM_FRAMES;
	}
	
	void Renderer::beginRenderPass(VkCommandBuffer commandBuffer)
	{
		assert(_isFrameStarted);
		assert(commandBuffer == getCurrentCommandBuffer());
	
		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext				= nullptr;
		renderPassBeginInfo.renderPass			= _swapChain->getRenderPass()->getHandle();
		renderPassBeginInfo.renderArea.offset	= { 0, 0 };
		renderPassBeginInfo.renderArea.extent	= _swapChain->getSwapChainExtent();
		renderPassBeginInfo.framebuffer			= _swapChain->getFramebuffer(_currentImageIndex)->getFramebufferHandle();
	
		VkClearValue clearColor = { {{0.0f, 0.0f, 0.1f, 1.0f}} };
		VkClearValue clearDepth = {};
		clearDepth.depthStencil.depth = 1.0f;
		VkClearValue clearValues[] = { clearColor, clearDepth };
		renderPassBeginInfo.clearValueCount = sizeof(clearValues) / sizeof(VkClearValue);
		renderPassBeginInfo.pClearValues	= clearValues;
	
		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	
		VkViewport viewport;
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = static_cast<float>(_swapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(_swapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	
		VkRect2D scissor;
		scissor.offset = { 0, 0 };
		scissor.extent = _swapChain->getSwapChainExtent();
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}
	
	void Renderer::endRenderPass(VkCommandBuffer commandBuffer)
	{
		assert(_isFrameStarted);
		assert(commandBuffer == getCurrentCommandBuffer());
		vkCmdEndRenderPass(commandBuffer);
	}

	void Renderer::addWaitSemaphore(VkSemaphore semaphore)
	{
		_waitSemaphores.emplace_back(semaphore);
	}

	void Renderer::addSignalSemaphore(VkSemaphore semaphore)
	{
		_signalSemaphores.emplace_back(semaphore);
	}
};