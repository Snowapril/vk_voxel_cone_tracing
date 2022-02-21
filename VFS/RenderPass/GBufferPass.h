// Author : Jihong Shin (snowapril)

#if !defined(VFS_GBUFFER_PASS_H)
#define VFS_GBUFFER_PASS_H

#include <RenderPass/RenderPassBase.h>

namespace vfs
{
	class GBufferPass : public RenderPassBase
	{
	public:
		explicit GBufferPass() = default;
		explicit GBufferPass(CommandPoolPtr cmdPool, VkExtent2D resolution);
		explicit GBufferPass(CommandPoolPtr cmdPool, VkExtent2D resolution,
							 const DescriptorSetLayoutPtr& globalDescLayout);
		~GBufferPass();

	public:
		bool initializeGBufferPass	(VkExtent2D resolution,
									 const DescriptorSetLayoutPtr& globalDescLayout);
		bool initializeDebugPass	(void);

		GBufferPass& createAttachments		(void);
		GBufferPass& createRenderPass		(void);
		GBufferPass& createFramebuffer		(void);
		GBufferPass& createPipeline			(const DescriptorSetLayoutPtr& globalDescLayout);
		
		void drawGUI		(void) override;
		void drawDebugInfo	(void) override;

		void processWindowResize(int width, int height) override;

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

	private:
		SamplerPtr		_colorSampler;
		VkExtent2D		_gbufferResolution{ 0, 0 };
		FramebufferPtr	_framebuffer;

		// Debug Info
		std::vector<VkDescriptorSet> _gbufferDebugDescSets;
	};
};

#endif