// Author : Jihong Shin (snowapril)

#if !defined(VFS_SWAPCHAIN_H)
#define VFS_SWAPCHAIN_H

#include <pch.h>
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
						   vfs::WindowPtr window,
						   VkSurfaceKHR surface);
		explicit SwapChain(std::unique_ptr<SwapChain>&& oldSwapChain);
				~SwapChain();
	
	public:
		void				destroySwapChain	(void);
		bool				initialize			(vfs::QueuePtr graphicsQueue,
												 vfs::QueuePtr presentQueue,
												 vfs::WindowPtr window,
												 VkSurfaceKHR surface);
		VkResult			submitCommandBuffer	(VkCommandBuffer* commandBuffer, uint32_t* imageIndex, 
												 std::vector<VkSemaphore> waitSemaphores,
												 std::vector<VkSemaphore> signalSemaphores);
		VkResult			acquireNextImage	(uint32_t* imageIndex);

		inline VkExtent2D		getSwapChainExtent(void) const
		{
			return _swapChainImageExtent;
		}
		inline VkFormat			getImageFormat(void) const
		{
			return _swapChainImageFormat;
		}
		inline uint32_t			getMinImageCount(void) const
		{
			return static_cast<uint32_t>(_swapChainImages.size());
		}
		inline WindowPtr		getWindowPtr(void)
		{
			return _window;
		}
		inline RenderPassPtr	getRenderPass(void)
		{
			return _renderPass;
		}
		inline FramebufferPtr	getFramebuffer(uint32_t index)
		{
			assert(index < static_cast<uint32_t>(_framebuffers.size()));
			return _framebuffers[index];
		}
		inline VkSurfaceKHR getSurfaceHandle(void)
		{
			return _surface;
		}
	private:
		bool				initializeSwapChain		(void);
		bool				initializeImageViews	(void);
		bool				initializeSyncObjects	(void);
		bool				initializeAttachments	(void);
		bool				initializeRenderPass	(void);
		bool				initializeFramebuffers	(void);
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
		RenderPassPtr					 _renderPass			{		nullptr		  };
		QueuePtr						 _graphicsQueue			{  		nullptr		  };
		QueuePtr						 _presentQueue			{  		nullptr		  };
		WindowPtr						 _window				{  		nullptr		  };
		std::unique_ptr<SwapChain>		 _oldSwapChain			{		nullptr		  };
		VkSurfaceKHR					 _surface				{	VK_NULL_HANDLE	  };
		ImagePtr						 _colorImage			{		nullptr		  };
		ImageViewPtr					 _colorImageView		{		nullptr		  };
		ImagePtr						 _depthImage			{		nullptr		  };
		ImageViewPtr					 _depthImageView		{		nullptr		  };
		VkSwapchainKHR					 _swapChainHandle		{	VK_NULL_HANDLE	  };
		VkFormat						 _swapChainImageFormat	{ VK_FORMAT_UNDEFINED };
		VkExtent2D						 _swapChainImageExtent	{ 0, 0 };
		uint32_t						 _currentFrameIndex		{ 0 };
	};
};

#endif