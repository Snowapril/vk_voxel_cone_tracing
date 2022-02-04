// Author : Jihong Shin (snowapril)

#if !defined(VFS_GBUFFER_PASS_H)
#define VFS_GBUFFER_PASS_H

#include <RenderPass/RenderPassBase.h>

namespace vfs
{
	struct FrameLayout;
	class GLTFScene;

	class GBufferPass : public RenderPassBase
	{
	public:
		explicit GBufferPass() = default;
		explicit GBufferPass(DevicePtr device, VkExtent2D resolution);
				~GBufferPass();

	public:
		GBufferPass& createAttachments		(void);
		GBufferPass& createRenderPass		(void);
		GBufferPass& createFramebuffer		(void);
		GBufferPass& createPipeline			(const DescriptorSetLayoutPtr& globalDescLayout,
											 const GLTFScene* scene);
		
		void drawUI(void) override;

		inline const FramebufferAttachment& getAttachment(uint32_t index) const
		{
			assert(index < _attachments.size());
			return _attachments[index];
		}
		inline const SamplerPtr& getColorAttachmentSampler(void) const
		{
			return _colorSampler;
		}
	private:
		void onBeginRenderPass	(const FrameLayout* frameLayout) override;
		void onEndRenderPass	(const FrameLayout* frameLayout) override;
		void onUpdate			(const FrameLayout* frameLayout) override;

		FramebufferAttachment createAttachment(VkExtent3D resolution, VkFormat format, VkImageUsageFlags usage);
	private:
		SamplerPtr		_colorSampler;
		VkExtent2D		_gbufferResolution{ 0, 0 };
		FramebufferPtr	_framebuffer;
	};
};

#endif