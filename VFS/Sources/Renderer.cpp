// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <Common/EngineConfig.h>
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
#include <iostream>
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
		destroyCommandBuffers();
		_swapChain.reset();
		_device.reset();
	}
	
	void Renderer::destroyCommandBuffers(void)
	{
		_mainCmdPool->freeCommandBuffers(_commandBuffers);
		_commandBuffers.clear();
	}
	
	bool Renderer::initialize(vfs::DevicePtr device,
							  vfs::CommandPoolPtr mainCmdPool,
							  std::unique_ptr<SwapChain>&& swapChain)
	{
		_device			= device;
		_mainCmdPool	= mainCmdPool;
		_swapChain		= std::move(swapChain);

		if (!initializeCommandBuffers())
		{
			std::cerr << "[RenderEngine] Failed to initialize command buffers\n";
			return false;
		}
	
		if (!initializeDepthImage())
		{
			return false;
		}
	
		if (!initializeRenderPass())
		{
			return false;
		}
	
		if (!initializeFramebuffers())
		{
			return false;
		}
	
		return true;
	}
	
	bool Renderer::recreateSwapChain(void)
	{
		vkDeviceWaitIdle(_device->getDeviceHandle());
		std::unique_ptr<SwapChain>&& tempSwapChain = std::move(_swapChain);
		_swapChain = std::make_unique<SwapChain>(std::move(tempSwapChain));
		return true;
	}
	
	bool Renderer::initializeCommandBuffers(void)
	{
		_commandBuffers = _mainCmdPool->allocateMultipleCommandBuffer(DEFAULT_NUM_FRAMES);
		return true;
	}
	
	bool Renderer::initializeDepthImage(void)
	{
		VkImageCreateInfo imageInfo = Image::GetDefaultImageCreateInfo();
		imageInfo.extent		= { _swapChain->getSwapChainExtent().width, _swapChain->getSwapChainExtent().height, 1 };
		// TODO(snowapril) : check depth format support for swapchain image
		imageInfo.format		= VK_FORMAT_D32_SFLOAT;
		imageInfo.usage			= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageInfo.imageType		= VK_IMAGE_TYPE_2D;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		_depthImages.resize(_swapChain->getImageCount());
		_depthImageViews.resize(_swapChain->getImageCount());

		for (uint32_t i = 0; i < _swapChain->getImageCount(); ++i)
		{
			_depthImages[i] = std::make_shared<Image>(_device->getMemoryAllocator(), VMA_MEMORY_USAGE_GPU_ONLY, imageInfo);
			_depthImageViews[i] = std::make_shared<vfs::ImageView>(_device, _depthImages[i], VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		}

		return true;
	}
	
	bool Renderer::initializeRenderPass(void)
	{
		VkAttachmentDescription colorAttachmentDesc = {};
		colorAttachmentDesc.format			= _swapChain->getImageFormat();
		colorAttachmentDesc.samples			= VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentDesc.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentDesc.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentDesc.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentDesc.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentDesc.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentDesc.finalLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	
		VkAttachmentDescription depthAttachmentDesc = {};
		depthAttachmentDesc.format			= VK_FORMAT_D32_SFLOAT;
		depthAttachmentDesc.samples			= VK_SAMPLE_COUNT_1_BIT;
		depthAttachmentDesc.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachmentDesc.storeOp			= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachmentDesc.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachmentDesc.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachmentDesc.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachmentDesc.finalLayout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	
		// TODO(snowapril) : Add VkSubPassDependency here
		VkSubpassDependency subpassDependency = {};
		subpassDependency.srcSubpass	= VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass	= 0;
		subpassDependency.srcStageMask	= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = 0;
		subpassDependency.dstStageMask	= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	
		_renderPass = std::make_shared<RenderPass>();
		assert(_renderPass->initialize(_device, { colorAttachmentDesc, depthAttachmentDesc }, { subpassDependency }, true));
		return true;
	}
	
	bool Renderer::initializeFramebuffers(void)
	{
		_framebuffers.reserve(_swapChain->getImageCount());
		for (uint32_t i = 0; i < _swapChain->getImageCount(); ++i)
		{
			std::vector<VkImageView> attachments = { _swapChain->getImageView(i), _depthImageViews[i]->getImageViewHandle() };
	
			vfs::FramebufferPtr framebuffer = std::make_shared<vfs::Framebuffer>();
			if (!framebuffer->initialize(_device, attachments, _renderPass->getHandle(), _swapChain->getSwapChainExtent()))
			{
				return false;
			}
			_framebuffers.emplace_back(std::move(framebuffer));
		}
		return true;
	}
	
	VkCommandBuffer Renderer::beginFrame(void)
	{
		assert(!_isFrameStarted);
	
		VkResult result = _swapChain->acquireNextImage(&_currentImageIndex);
		// TODO(snowapril) : handling result from acquireNextImage (timeout, expired, etc..)
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
			std::cerr << "[RenderEngine] Failed to begin command buffer\n";
			return VK_NULL_HANDLE;
		}
	
		_isFrameStarted = true;
		return currentCommandBuffer;
	}
	
	void Renderer::endFrame(void)
	{
		assert(_isFrameStarted);
	
		VkCommandBuffer& currentCommandBuffer = getCurrentCommandBuffer();
		// TODO(snowapril) : How to deal with VK_RESULT of below ?
		vkEndCommandBuffer(currentCommandBuffer);

		VkResult result = _swapChain->submitCommandBuffer(&currentCommandBuffer, &_currentImageIndex,
														  _waitSemaphores, _signalSemaphores);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			// TODO(snowapril) : glfw window resizing
			recreateSwapChain();
		}
		// TODO(snowapril) : handling result from submitCommandBuffer (timeout, expired, etc..)
		
		_isFrameStarted = false;
		_currentFrameIndex = (_currentFrameIndex + 1) % DEFAULT_NUM_FRAMES;
	}
	
	void Renderer::beginRenderPass(VkCommandBuffer commandBuffer)
	{
		assert(_isFrameStarted);
		assert(commandBuffer == getCurrentCommandBuffer());
	
		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = _renderPass->getHandle();
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = _swapChain->getSwapChainExtent();
		renderPassBeginInfo.framebuffer = _framebuffers[_currentImageIndex]->getFramebufferHandle();
	
		// TODO(snowapril) : modify this to clear screen with desired color
		VkClearValue clearColor = { {{0.0f, 0.0f, 0.3f, 1.0f}} };
		VkClearValue clearDepth = {};
		clearDepth.depthStencil.depth = 1.0f;
		VkClearValue clearValues[] = { clearColor, clearDepth };
		renderPassBeginInfo.clearValueCount = sizeof(clearValues) / sizeof(VkClearValue);
		renderPassBeginInfo.pClearValues = clearValues;
	
		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	
		VkViewport viewport;
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = static_cast<float>(_swapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(_swapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
	
		VkRect2D scissor;
		scissor.offset = { 0, 0 };
		scissor.extent = _swapChain->getSwapChainExtent();
	
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
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