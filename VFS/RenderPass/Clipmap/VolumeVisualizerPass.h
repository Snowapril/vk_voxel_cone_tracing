// Author : Jihong Shin (snowapril)

#if !defined(VFS_VOLUME_VISUALIZER_PASS_H)
#define VFS_VOLUME_VISUALIZER_PASS_H

#include <RenderPass/RenderPassBase.h>

namespace vfs
{
	struct FrameLayout;
	class GLTFScene;

	class VolumeVisualizerPass : public RenderPassBase
	{
	public:
		explicit VolumeVisualizerPass() = default;
		explicit VolumeVisualizerPass(CommandPoolPtr cmdPool, VkExtent2D resolution);
				~VolumeVisualizerPass();

	public:
		VolumeVisualizerPass& createAttachments(void);
		VolumeVisualizerPass& createRenderPass(void);
		VolumeVisualizerPass& createFramebuffer(void);
		VolumeVisualizerPass& createPipeline(const DescriptorSetLayoutPtr& globalDescLayout,
			const GLTFScene* scene);

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
	};
};

#endif