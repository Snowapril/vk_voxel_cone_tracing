// Author : Jihong Shin (snowapril)

#if !defined(VFS_SHADOW_MAP_PASS_H)
#define VFS_SHADOW_MAP_PASS_H

#include <RenderPass/RenderPassBase.h>
#include <DirectionalLight.h>

namespace vfs
{
	class GLTFScene;
	class ShadowMapPass : public RenderPassBase
	{
	public:
		explicit ShadowMapPass() = default;
		explicit ShadowMapPass(DevicePtr device, VkExtent2D resolution);
				~ShadowMapPass();

	public:
		ShadowMapPass& createRenderPass		(void);
		ShadowMapPass& setDirectionalLight	(std::unique_ptr<DirectionalLight>&& dirLight);
		ShadowMapPass& createPipeline		(const DescriptorSetLayoutPtr& globalDescLayout,
											 const GLTFScene* scene);
		
		void drawUI(void) override;

		inline const FramebufferAttachment& getDepthAttachment(void) const
		{
			assert(_attachments.empty() == false);
			return _attachments.front();
		}
	private:
		void onBeginRenderPass	(const FrameLayout* frameLayout) override;
		void onEndRenderPass	(const FrameLayout* frameLayout) override;
		void onUpdate			(const FrameLayout* frameLayout) override;

	private:
		std::unique_ptr<DirectionalLight>				_directionalLight;
		FramebufferPtr									_framebuffer;
		VkExtent2D				_shadowMapResolution	{ 0, 0 };
		BufferPtr				_viewProjBuffer			{ nullptr };
		DescriptorSetPtr		_descriptorSet			{ nullptr };
		DescriptorPoolPtr		_descriptorPool			{ nullptr };
		DescriptorSetLayoutPtr	_descriptorLayout		{ nullptr };
	};
};

#endif