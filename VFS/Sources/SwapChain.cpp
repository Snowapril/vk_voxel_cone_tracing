// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <Common/EngineConfig.h>
#include <Common/Utils.h>
#include <SwapChain.h>
#include <VulkanFramework/Window.h>
#include <VulkanFramework/Framebuffer.h>
#include <VulkanFramework/Semaphore.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/Device.h>
#include <iostream>

namespace vfs
{
	SwapChain::SwapChain(vfs::QueuePtr graphicsQueue,
						 vfs::QueuePtr presentQueue,
						 vfs::WindowPtr window)
	{
		_oldSwapChain = nullptr;
		assert(initialize(graphicsQueue, presentQueue, window));
	}
	
	SwapChain::SwapChain(std::unique_ptr<SwapChain>&& oldSwapChain)
	{
		_oldSwapChain = std::move(oldSwapChain);
		assert(initialize(_oldSwapChain->_graphicsQueue, _oldSwapChain->_presentQueue, _oldSwapChain->_window));
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
		_window.reset();
		_device.reset();
	}
	
	bool SwapChain::initialize(vfs::QueuePtr graphicsQueue,
							   vfs::QueuePtr presentQueue,
							   vfs::WindowPtr window)
	{
		_graphicsQueue	= graphicsQueue;
		_presentQueue	= presentQueue;
		_device			= _graphicsQueue->getDevicePtr();
		_window			= window;
	
		if (!initializeSwapChain())
		{
			std::cerr << "[RenderEngine] Failed to create swap chain\n";
			return false;
		}
	
		if (!initializeImageViews())
		{
			std::cerr << "[RenderEngine] Failed to create image views\n";
			return false;
		}
		if (!initializeSyncObjects())
		{
			std::cerr << "[RenderEngine] Failed to create synchronize objects\n";
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
	
		//VkPipelineStageFlags stageFlags[2] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
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
		vfs::Device::SwapChainSupportDetails detail	= _device->querySwapChainSupport();
		VkSurfaceFormatKHR surfaceFormat		= pickSwapSurfaceFormat(detail.formats);
		VkPresentModeKHR presentMode			= pickSwapPresentMode(detail.presentModes);
		VkExtent2D extent						= pickSwapExtent(detail.capabilities);
	
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
		swapChainInfo.surface			= _device->getSurfaceHandle();
		swapChainInfo.minImageCount		= minImageCount;
		swapChainInfo.imageColorSpace	= surfaceFormat.colorSpace;
		swapChainInfo.imageFormat		= surfaceFormat.format;
		swapChainInfo.imageExtent		= extent;
		swapChainInfo.imageArrayLayers	= 1;
		swapChainInfo.imageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	
		uint32_t graphicsFamily{ 0 }, presentFamily{ 0 }, loaderFamily{ 0 };
		_device->findQueueFamilyIndices(&graphicsFamily, &presentFamily, &loaderFamily);
		const uint32_t queueFamilyIndices[] = { graphicsFamily, presentFamily };
		if (graphicsFamily != presentFamily)
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
			std::cerr << "[RenderEngine] Failed to create swapchain\n";
			return false;
		}
		std::clog << "[RenderEngine] Swap chain created\n";
	
		vkGetSwapchainImagesKHR(_device->getDeviceHandle(), _swapChainHandle, &minImageCount, nullptr);
		_swapChainImages.resize(minImageCount);
		vkGetSwapchainImagesKHR(_device->getDeviceHandle(), _swapChainHandle, &minImageCount, _swapChainImages.data());
		std::clog << "[RenderEngine] Swap chain images retrieved\n";
	
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
	
		std::clog << "[RenderEngine] Swap chain image views created\n";
		return true;
	}
	
	bool SwapChain::initializeSyncObjects(void)
	{
		VkDevice device = _device->getDeviceHandle();
	
		_imageAvailableSemaphores.reserve(vfs::DEFAULT_NUM_FRAMES);
		_renderFinishedSemaphores.reserve(vfs::DEFAULT_NUM_FRAMES);
		_inFlightFences.reserve(vfs::DEFAULT_NUM_FRAMES);
		_imagesInFlight.resize(getImageCount(), nullptr);
	
		for (size_t i = 0; i < vfs::DEFAULT_NUM_FRAMES; ++i)
		{
			_imageAvailableSemaphores.emplace_back(std::make_unique<vfs::Semaphore>(_device));
			_renderFinishedSemaphores.emplace_back(std::make_unique<vfs::Semaphore>(_device));
			_inFlightFences.emplace_back(std::make_shared<vfs::Fence>(_device, 1, VK_FENCE_CREATE_SIGNALED_BIT));
		}
		std::clog << "[RenderEngine] Synchronize objects(semaphore, fence) created\n";
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
			if (presentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR) 
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
};