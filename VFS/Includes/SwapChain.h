// Author : Jihong Shin (snowapril)

#if !defined(VFS_SWAPCHAIN_H)
#define VFS_SWAPCHAIN_H

#include <Common/pch.h>
#include <VulkanFramework/Sync/Fence.h>
#include <VulkanFramework/Sync/Semaphore.h>

namespace vfs
{
	class SwapChain : vfs::NonCopyable
	{
	public:
		explicit SwapChain() = default;
		explicit SwapChain(vfs::QueuePtr graphicsQueue,
						   vfs::QueuePtr presentQueue,
						   vfs::WindowPtr window);
		explicit SwapChain(std::unique_ptr<SwapChain>&& oldSwapChain);
				~SwapChain();
	
	public:
		void				destroySwapChain	(void);
		bool				initialize			(vfs::QueuePtr graphicsQueue,
												 vfs::QueuePtr presentQueue,
												 vfs::WindowPtr window);
		VkResult			submitCommandBuffer	(VkCommandBuffer* commandBuffer, uint32_t* imageIndex, 
												 std::vector<VkSemaphore> waitSemaphores,
												 std::vector<VkSemaphore> signalSemaphores);
		VkResult			acquireNextImage	(uint32_t* imageIndex);

		inline VkExtent2D		getSwapChainExtent(void) const
		{
			return _swapChainImageExtent;
		}
		inline uint32_t			getMinImageCount(void) const
		{
			return _minImageCount;
		}
		inline uint32_t			getImageCount(void) const
		{
			return static_cast<uint32_t>(_swapChainImages.size());
		}
		inline VkFormat			getImageFormat(void) const
		{
			return _swapChainImageFormat;
		}
		inline VkImageView getImageView(uint32_t i) const
		{
			assert(i < _swapChainImageViews.size());
			return _swapChainImageViews[i];
		}
	private:
		bool				initializeSwapChain		(void);
		bool				initializeImageViews	(void);
		bool				initializeSyncObjects	(void);
		VkSurfaceFormatKHR	pickSwapSurfaceFormat	(const std::vector<VkSurfaceFormatKHR>& formats);
		VkPresentModeKHR	pickSwapPresentMode		(const std::vector<VkPresentModeKHR>& presentModes);
		VkExtent2D			pickSwapExtent			(VkSurfaceCapabilitiesKHR capabilities);

		struct SwapChainSupportDetails
		{
			VkSurfaceCapabilitiesKHR		capabilities{};
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR>	presentModes;
		};
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice);
	private:
		std::vector<vfs::FramebufferPtr> _framebuffers;
		std::vector<VkImage>			 _swapChainImages;
		std::vector<VkImageView>		 _swapChainImageViews;
		std::vector<std::unique_ptr<vfs::Semaphore>> _imageAvailableSemaphores;
		std::vector<std::unique_ptr<vfs::Semaphore>> _renderFinishedSemaphores;
		std::vector<vfs::FencePtr>		 _inFlightFences;
		std::vector<vfs::Fence*  >		 _imagesInFlight;
		DevicePtr						 _device				{  		nullptr		  };
		QueuePtr						 _graphicsQueue			{  		nullptr		  };
		QueuePtr						 _presentQueue			{  		nullptr		  };
		WindowPtr						 _window				{  		nullptr		  };
		std::unique_ptr<SwapChain>		 _oldSwapChain			{		nullptr		  };
		VkSurfaceKHR					 _surface				{ VK_NULL_HANDLE };
		VkSwapchainKHR					 _swapChainHandle		{	VK_NULL_HANDLE	  };
		VkFormat						 _swapChainImageFormat	{ VK_FORMAT_UNDEFINED };
		VkExtent2D						 _swapChainImageExtent	{ 0, 0 };
		uint32_t						 _currentFrameIndex		{ 0 };
		uint32_t						 _minImageCount			{ 0 };
	};
};

#endif