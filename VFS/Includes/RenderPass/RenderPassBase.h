// Author : Jihong Shin (snowapril)

#if !defined(VFS_RENDER_PASS_BASE_H)
#define VFS_RENDER_PASS_BASE_H

#include <Common/pch.h>
#include <Common/VertexFormat.h>
#include <VulkanFramework/DebugUtils.h>

namespace vfs
{
	struct FrameLayout;
	class RenderPassManager;

	class RenderPassBase : NonCopyable
	{
	public:
		explicit RenderPassBase() = default;
		explicit RenderPassBase(DevicePtr device);
		explicit RenderPassBase(DevicePtr device,
								RenderPassPtr renderPass);
		virtual ~RenderPassBase();

		struct FramebufferAttachment
		{
			ImagePtr		image;
			ImageViewPtr	imageView;
		};
	public:
		void beginRenderPass		(const FrameLayout* frameLayout);
		void endRenderPass			(const FrameLayout* frameLayout);
		void update					(const FrameLayout* frameLayout);
		void attachRenderPassManager(RenderPassManager* renderPassManager);
		
		// UI Rendering
		virtual void drawUI(void) {};

		// Input callbacks
		virtual void processKeyInput	 (uint32_t key, bool pressed)	{};
		virtual void processCursorPos	 (double xpos, double ypos)		{};
		virtual void processWindowResize (int width, int height)		{};


		inline PipelineLayoutPtr getPipelineLayout(void) const
		{
			return _pipelineLayout;
		}
		inline RenderPassPtr	 getRenderPass(void) const
		{
			return _renderPass;
		}
	protected:
		virtual void onBeginRenderPass	(const FrameLayout* frameLayout) = 0;
		virtual void onEndRenderPass	(const FrameLayout* frameLayout) = 0;
		virtual void onUpdate			(const FrameLayout* frameLayout) = 0;

	protected:
		RenderPassManager*					_renderPassManager { nullptr};
		DevicePtr							_device;
		RenderPassPtr						_renderPass;
		GraphicsPipelinePtr					_pipeline;
		PipelineLayoutPtr					_pipelineLayout;
		std::vector<FramebufferAttachment>	_attachments;
		DebugUtils							_debugUtils;
	};
};

#endif