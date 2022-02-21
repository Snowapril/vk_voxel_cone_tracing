// Author : Jihong Shin (snowapril)

#if !defined(VFS_RENDERER_H)
#define VFS_RENDERER_H

#include <pch.h>
#include <SwapChain.h>
#include <memory>

namespace vfs
{
	class Renderer : NonCopyable
	{
	public:
		explicit Renderer() = default;
		explicit Renderer(vfs::DevicePtr device,
						  vfs::CommandPoolPtr mainCmdPool,
						  std::unique_ptr<SwapChain>&& swapChain);
				~Renderer();
	
	public:
		void			destroyRenderer			(void);
		bool			initialize				(vfs::DevicePtr device,
												 vfs::CommandPoolPtr mainCmdPool,
												 std::unique_ptr<SwapChain>&& swapChain);
		bool			recreateSwapChain		(void);
		VkCommandBuffer beginFrame				(void);
		void			endFrame				(void);
		void			beginRenderPass			(VkCommandBuffer commandBuffer);
		void			endRenderPass			(VkCommandBuffer commandBuffer);
		void			addWaitSemaphore		(VkSemaphore semaphore);
		void			addSignalSemaphore		(VkSemaphore semaphore);

		inline VkCommandBuffer& getCurrentCommandBuffer(void)
		{
			return _commandBuffers[_currentFrameIndex];
		}
		inline uint32_t			getCurrentFrameIndex(void) const
		{
			return _currentFrameIndex;
		}
		inline uint32_t			getFrameCount(void) const
		{
			return _swapChain->getMinImageCount();
		}
		inline RenderPassPtr	getSwapChainRenderPass(void) const
		{
			return _swapChain->getRenderPass();
		}
	
	private:
		std::vector<VkCommandBuffer>		_commandBuffers;
		std::vector<VkSemaphore>			_waitSemaphores;
		std::vector<VkSemaphore>			_signalSemaphores;
		DevicePtr							_device				{ nullptr };
		CommandPoolPtr						_mainCmdPool		{ nullptr };
		std::unique_ptr<SwapChain>			_swapChain			{ nullptr };
		VkSurfaceKHR						_surface			{ VK_NULL_HANDLE };
		uint32_t							_currentImageIndex	{ 0 };
		uint32_t							_currentFrameIndex	{ 0 };
		bool								_isFrameStarted		{ false };
	};
};

#endif