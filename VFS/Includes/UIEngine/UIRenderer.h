// Author : Jihong Shin (snowapril)

#if !defined(UI_ENGINE_UI_RENDERER_H)
#define UI_ENGINE_UI_RENDERER_H

#include <Common/pch.h>
#include <memory>

namespace vfs
{
	struct FrameLayout;

	class UIRenderer : NonCopyable
	{
	public:
		explicit UIRenderer() = default;
		explicit UIRenderer(std::shared_ptr<Window> window, 
							std::shared_ptr<Device> device, 
							QueuePtr graphicsQueue,
							VkRenderPass renderPass);
				~UIRenderer();

	public:
		void destroyUIRenderer	(void);
		bool initialize			(std::shared_ptr<Window> window, 
								 std::shared_ptr<Device> device,
								 QueuePtr graphicsQueue,
								 VkRenderPass renderPass);
		bool createFontTexture	(const CommandPoolPtr& cmdPool);
		void beginUIRender		(void);
		void endUIRender		(const FrameLayout* frameLayout);
	private:
		bool initializeDescriptorPool	(void);
		bool initializeImGuiContext		(std::shared_ptr<Window> window, VkRenderPass renderPass);

	private:
		std::shared_ptr<Device> _device;
		QueuePtr		 _graphicsQueue	{ nullptr };
		VkDescriptorPool _descriptorPool{ VK_NULL_HANDLE };
	};
}

#endif