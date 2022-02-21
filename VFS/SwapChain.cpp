// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <Util/EngineConfig.h>
#include <Common/Logger.h>
#include <Common/Utils.h>
#include <SwapChain.h>
#include <VulkanFramework/Window.h>
#include <VulkanFramework/RenderPass/Framebuffer.h>
#include <VulkanFramework/Sync/Semaphore.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Utils.h>
#include <VulkanFramework/Images/Image.h>
#include <VulkanFramework/Images/ImageView.h>
#include <VulkanFramework/RenderPass/RenderPass.h>

namespace vfs
{
	SwapChain::SwapChain(vfs::QueuePtr graphicsQueue,
						 vfs::QueuePtr presentQueue,
						 vfs::WindowPtr window,
						 VkSurfaceKHR surface)
	{
		_oldSwapChain = nullptr;
		assert(initialize(graphicsQueue, presentQueue, window, surface));
	}
	
	SwapChain::SwapChain(std::unique_ptr<SwapChain>&& oldSwapChain)
	{
		_oldSwapChain	= std::move(oldSwapChain);
		assert(initialize(_oldSwapChain->_graphicsQueue, _oldSwapChain->_presentQueue, 
						  _oldSwapChain->_window, _oldSwapChain->_surface));
	}
	
	SwapChain::~SwapChain()
	{
		destroySwapChain();
	}
	
	void SwapChain::destroySwapChain(void)
	{
		const VkDevice device = _device->getDeviceHandle();
		_imageAvailableSemaphores.clear();
		_renderFinishedSemaphores.clear();
		_inFlightFences.clear();
		_framebuffers.clear();
		for (VkImageView& view : _swapChainImageViews)
		{
			vkDestroyImageView(device, view, nullptr);
		}
		_swapChainImageViews.clear();
		_swapChainImages.clear();
		vkDestroySwapchainKHR(device, _swapChainHandle, nullptr);
		_renderPass.reset();
		_window.reset();
		_device.reset();
	}
	
	bool SwapChain::initialize(vfs::QueuePtr graphicsQueue,
							   vfs::QueuePtr presentQueue,
							   vfs::WindowPtr window,
							   VkSurfaceKHR surface)
	{
		_graphicsQueue	= graphicsQueue;
		_presentQueue	= presentQueue;
		_device			= _graphicsQueue->getDevicePtr();
		_window			= window;
		_surface		= surface;
	
		if (!initializeSwapChain())
		{
			VFS_ERROR << "Failed to create swap chain";
			return false;
		}
	
		if (!initializeImageViews())
		{
			VFS_ERROR << "Failed to create image views";
			return false;
		}

		if (!initializeSyncObjects())
		{
			VFS_ERROR << "Failed to create synchronize objects";
			return false;
		}

		if (!initializeAttachments())
		{
			VFS_ERROR << "Failed to create framebuffer attachments";
			return false;
		}

		if (!initializeRenderPass())
		{
			VFS_ERROR << "Failed to create renderpass";
			return false;
		}

		if (!initializeFramebuffers())
		{
			VFS_ERROR << "Failed to create framebuffers";
			return false;
		}

		return true;
	}
	
	VkResult SwapChain::submitCommandBuffer(VkCommandBuffer* commandBuffer, uint32_t* imageIndex,
											std::vector<VkSemaphore> waitSemaphores,
											std::vector<VkSemaphore> signalSemaphores)
	{
		if (_imagesInFlight[*imageIndex] != nullptr)
		{
			_imagesInFlight[*imageIndex]->waitForAllFences(UINT64_MAX);
		}
		_imagesInFlight[*imageIndex] = _inFlightFences[_currentFrameIndex].get();
	
		waitSemaphores.emplace_back(_imageAvailableSemaphores[_currentFrameIndex]->getHandle());
		signalSemaphores.emplace_back(_renderFinishedSemaphores[_currentFrameIndex]->getHandle());

		VkSubmitInfo submitInfo = {};
		submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext				= nullptr;
		submitInfo.commandBufferCount	= 1;
		submitInfo.pCommandBuffers		= commandBuffer;
		submitInfo.waitSemaphoreCount	= static_cast<uint32_t>(waitSemaphores.size());
		submitInfo.pWaitSemaphores		= waitSemaphores.data();
		submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
		submitInfo.pSignalSemaphores	= signalSemaphores.data();
	
		VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		submitInfo.pWaitDstStageMask = &stageFlags;
	
		VkResult result = vkResetFences(_device->getDeviceHandle(), 1, &_inFlightFences[_currentFrameIndex]->getFence(0));
		if (result != VK_SUCCESS)
		{
			return result;
		}

		result = vkQueueSubmit(_graphicsQueue->getQueueHandle(), 1, &submitInfo, _inFlightFences[_currentFrameIndex]->getFence(0));
		if (result != VK_SUCCESS)
		{
			return result;
		}
		
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext				= nullptr;
		presentInfo.swapchainCount		= 1;
		presentInfo.pSwapchains			= &_swapChainHandle;
		presentInfo.pImageIndices		= imageIndex;
		presentInfo.waitSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
		presentInfo.pWaitSemaphores		= signalSemaphores.data();
		presentInfo.pResults			= nullptr;

		result = vkQueuePresentKHR(_presentQueue->getQueueHandle(), &presentInfo);
		if (result != VK_SUCCESS)
		{
			return result;
		}

		_currentFrameIndex = (_currentFrameIndex + 1) % vfs::DEFAULT_NUM_FRAMES;
		return result;
	}
	
	VkResult SwapChain::acquireNextImage(uint32_t* imageIndex)
	{
		_inFlightFences[_currentFrameIndex]->waitForAllFences(UINT64_MAX);
	
		VkResult result = vkAcquireNextImageKHR(
			_device->getDeviceHandle(),
			_swapChainHandle,
			UINT64_MAX,
			_imageAvailableSemaphores[_currentFrameIndex]->getHandle(),
			VK_NULL_HANDLE,
			imageIndex
		);

		return result;
	}

	bool SwapChain::initializeSwapChain(void)
	{
		SwapChain::SwapChainSupportDetails detail	= querySwapChainSupport(_device->getPhysicalDeviceHandle());
		VkSurfaceFormatKHR surfaceFormat			= pickSwapSurfaceFormat(detail.formats);
		VkPresentModeKHR presentMode				= pickSwapPresentMode(detail.presentModes);
		VkExtent2D extent							= pickSwapExtent(detail.capabilities);

		_swapChainImageFormat = surfaceFormat.format;
		_swapChainImageExtent = extent;
	
		uint32_t minImageCount = detail.capabilities.minImageCount + 1;
		if (detail.capabilities.maxImageCount > 0 && minImageCount > detail.capabilities.maxImageCount)
		{
			minImageCount = detail.capabilities.maxImageCount;
		}
	
		VkSwapchainCreateInfoKHR swapChainInfo = {};
		swapChainInfo.sType				= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainInfo.pNext				= nullptr;
		swapChainInfo.surface			= _surface;
		swapChainInfo.minImageCount		= minImageCount;
		swapChainInfo.imageColorSpace	= surfaceFormat.colorSpace;
		swapChainInfo.imageFormat		= surfaceFormat.format;
		swapChainInfo.imageExtent		= extent;
		swapChainInfo.imageArrayLayers	= 1;
		swapChainInfo.imageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	
		const uint32_t queueFamilyIndices[] = { _graphicsQueue->getFamilyIndex(), _presentQueue->getFamilyIndex()};
		if (queueFamilyIndices[0] != queueFamilyIndices[1])
		{
			swapChainInfo.imageSharingMode		= VK_SHARING_MODE_CONCURRENT;
			swapChainInfo.queueFamilyIndexCount = 2;
			swapChainInfo.pQueueFamilyIndices	= queueFamilyIndices;
		}
		else
		{
			swapChainInfo.imageSharingMode		= VK_SHARING_MODE_EXCLUSIVE;
			swapChainInfo.queueFamilyIndexCount = 0;
			swapChainInfo.pQueueFamilyIndices	= nullptr;
		}
	
		swapChainInfo.preTransform	 = detail.capabilities.currentTransform;
		swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	
		swapChainInfo.presentMode	= presentMode;
		swapChainInfo.clipped		= VK_TRUE;
		swapChainInfo.oldSwapchain	= _oldSwapChain == nullptr ? VK_NULL_HANDLE : _oldSwapChain->_swapChainHandle;
	
		if (vkCreateSwapchainKHR(_device->getDeviceHandle(), &swapChainInfo, nullptr, &_swapChainHandle) != VK_SUCCESS)
		{
			return false;
		}
	
		vkGetSwapchainImagesKHR(_device->getDeviceHandle(), _swapChainHandle, &minImageCount, nullptr);
		_swapChainImages.resize(minImageCount);
		vkGetSwapchainImagesKHR(_device->getDeviceHandle(), _swapChainHandle, &minImageCount, _swapChainImages.data());
		return true;
	}
	
	bool SwapChain::initializeImageViews(void)
	{
		_swapChainImageViews.resize(_swapChainImages.size());
		const VkDevice device = _device->getDeviceHandle();
		for (size_t i = 0; i < _swapChainImages.size(); ++i)
		{
			VkImageViewCreateInfo imageViewInfo = {};
			imageViewInfo.sType			= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewInfo.pNext			= nullptr;
			imageViewInfo.viewType		= VK_IMAGE_VIEW_TYPE_2D;
			imageViewInfo.format		= _swapChainImageFormat;
			imageViewInfo.image			= _swapChainImages[i];
			imageViewInfo.components.r	= VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewInfo.components.g	= VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewInfo.components.b	= VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewInfo.components.a	= VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewInfo.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewInfo.subresourceRange.baseMipLevel		= 0;
			imageViewInfo.subresourceRange.levelCount		= 1;
			imageViewInfo.subresourceRange.baseArrayLayer	= 0;
			imageViewInfo.subresourceRange.layerCount		= 1;
	
			if (vkCreateImageView(device, &imageViewInfo, nullptr, &_swapChainImageViews[i]) != VK_SUCCESS)
			{
				return false;
			}
		}
		return true;
	}
	
	bool SwapChain::initializeSyncObjects(void)
	{
		VkDevice device = _device->getDeviceHandle();
	
		_imageAvailableSemaphores.reserve(vfs::DEFAULT_NUM_FRAMES);
		_renderFinishedSemaphores.reserve(vfs::DEFAULT_NUM_FRAMES);
		_inFlightFences.reserve(vfs::DEFAULT_NUM_FRAMES);
		_imagesInFlight.resize(_swapChainImages.size(), nullptr);
	
		for (size_t i = 0; i < vfs::DEFAULT_NUM_FRAMES; ++i)
		{
			_imageAvailableSemaphores.emplace_back(std::make_unique<vfs::Semaphore>(_device));
			_renderFinishedSemaphores.emplace_back(std::make_unique<vfs::Semaphore>(_device));
			_inFlightFences.emplace_back(std::make_shared<vfs::Fence>(_device, 1, VK_FENCE_CREATE_SIGNALED_BIT));
		}
		return true;
	}
	
	bool SwapChain::initializeAttachments(void)
	{
		VkImageCreateInfo colorImageInfo = Image::GetDefaultImageCreateInfo();
		colorImageInfo.extent			= { _swapChainImageExtent.width, _swapChainImageExtent.height, 1 };
		colorImageInfo.format			= _swapChainImageFormat;
		colorImageInfo.usage			= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
		colorImageInfo.imageType		= VK_IMAGE_TYPE_2D;
		colorImageInfo.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		colorImageInfo.samples			= getMaximumSampleCounts(_device);

		_colorImage		 = std::make_shared<Image>(_device->getMemoryAllocator(), VMA_MEMORY_USAGE_GPU_ONLY, colorImageInfo);
		_colorImageView  = std::make_shared<vfs::ImageView>(_device, _colorImage, VK_IMAGE_ASPECT_COLOR_BIT, 1);

		VkImageCreateInfo depthImageInfo = Image::GetDefaultImageCreateInfo();
		depthImageInfo.extent			= { _swapChainImageExtent.width, _swapChainImageExtent.height, 1 };
		depthImageInfo.format			= VK_FORMAT_D32_SFLOAT;
		depthImageInfo.usage			= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depthImageInfo.imageType		= VK_IMAGE_TYPE_2D;
		depthImageInfo.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		depthImageInfo.samples			= getMaximumSampleCounts(_device);

		_depthImage		 = std::make_shared<Image>(_device->getMemoryAllocator(), VMA_MEMORY_USAGE_GPU_ONLY, depthImageInfo);
		_depthImageView  = std::make_shared<vfs::ImageView>(_device, _depthImage, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

		return true;
	}
	
	bool SwapChain::initializeRenderPass(void)
	{
		VkAttachmentDescription colorAttachmentDesc = {};
		colorAttachmentDesc.format			= _swapChainImageFormat;
		colorAttachmentDesc.samples			= getMaximumSampleCounts(_device);
		colorAttachmentDesc.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentDesc.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentDesc.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentDesc.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentDesc.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentDesc.finalLayout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
		VkAttachmentDescription depthAttachmentDesc = {};
		depthAttachmentDesc.format			= VK_FORMAT_D32_SFLOAT;
		depthAttachmentDesc.samples			= getMaximumSampleCounts(_device);
		depthAttachmentDesc.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachmentDesc.storeOp			= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachmentDesc.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachmentDesc.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachmentDesc.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachmentDesc.finalLayout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachmentResolve = {};
		colorAttachmentResolve.format			= _swapChainImageFormat;
		colorAttachmentResolve.samples			= VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp			= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	
		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentRef.attachment	= 0;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachmentRef.attachment	= 1;

		VkAttachmentReference resolveAttachmentRef = {};
		resolveAttachmentRef.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		resolveAttachmentRef.attachment = 2;

		VkSubpassDescription subpassDesc = {};
		subpassDesc.colorAttachmentCount	= 1;
		subpassDesc.pColorAttachments		= &colorAttachmentRef;
		subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;
		subpassDesc.pResolveAttachments		= &resolveAttachmentRef;
		subpassDesc.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;

		// TODO(snowapril) : Add VkSubPassDependency here
		VkSubpassDependency subpassDependency = {};
		subpassDependency.srcSubpass	= VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass	= 0;
		subpassDependency.srcStageMask	= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = 0;
		subpassDependency.dstStageMask	= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	
		_renderPass = std::make_shared<RenderPass>();
		assert(_renderPass->initialize(_device, { colorAttachmentDesc, depthAttachmentDesc, colorAttachmentResolve }, { subpassDependency }, { subpassDesc }));
		return true;
	}
	
	bool SwapChain::initializeFramebuffers(void)
	{
		_framebuffers.reserve(_swapChainImages.size());
		for (size_t i = 0; i < _swapChainImages.size(); ++i)
		{
			std::vector<VkImageView> attachments = { _colorImageView->getImageViewHandle(), _depthImageView->getImageViewHandle(), _swapChainImageViews[i] };
	
			vfs::FramebufferPtr framebuffer = std::make_shared<vfs::Framebuffer>();
			if (!framebuffer->initialize(_device, attachments, _renderPass->getHandle(), _swapChainImageExtent))
			{
				return false;
			}
			_framebuffers.emplace_back(std::move(framebuffer));
		}
		return true;
	}

	VkSurfaceFormatKHR SwapChain::pickSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
	{
		assert(formats.empty() == false); // snowapril : given formats parameter must not be empty
	
		for (const VkSurfaceFormatKHR& availableFormat : formats)
		{
			if (availableFormat.format		== VK_FORMAT_B8G8R8A8_SRGB &&
				availableFormat.colorSpace  == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}
		return formats.front();
	}
	
	VkPresentModeKHR SwapChain::pickSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes)
	{
		assert(presentModes.empty() == false); // snowapril : given presentModes must not be empty
	
		for (const VkPresentModeKHR& presentMode : presentModes)
		{
			// TODO(snowapril) : use VK_PRESENT_MODE_MAILBOX_KHR instead, but sync may failed in my code (must fix)
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) // VK_PRESENT_MODE_FIFO_RELAXED_KHR
			{
				return presentMode;
			}
		}
	
		return presentModes.front();
	}
	
	VkExtent2D SwapChain::pickSwapExtent(VkSurfaceCapabilitiesKHR capabilities)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			return capabilities.currentExtent;
		}
		else
		{
			int width{ 0 }, height{ 0 };
			glfwGetFramebufferSize(_window->getWindowHandle(), &width, &height);
	
			VkExtent2D extent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height),
			};
			extent.width = vfs::clamp(
				extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width
			);
			extent.height = vfs::clamp(
				extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height
			);
	
			return extent;
		}
	}

	SwapChain::SwapChainSupportDetails SwapChain::querySwapChainSupport(VkPhysicalDevice physicalDevice)
	{
		SwapChain::SwapChainSupportDetails detail;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, _surface, &detail.capabilities);

		uint32_t surfaceFormatCount{ 0 };
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, &surfaceFormatCount, nullptr);

		if (surfaceFormatCount != 0)
		{
			detail.formats.resize(surfaceFormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, &surfaceFormatCount, detail.formats.data());
		}

		uint32_t presentModeCount{ 0 };
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			detail.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, &presentModeCount, detail.presentModes.data());
		}

		return detail;
	}
};