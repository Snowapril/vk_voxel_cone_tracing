// Author : Jihong Shin (snowapril)

#if !defined(VFS_REFLECTIVE_SHADOW_MAP_PASS_H)
#define VFS_REFLECTIVE_SHADOW_MAP_PASS_H

#include <RenderPass/RenderPassBase.h>
#include <DirectionalLight.h>

namespace vfs
{
	class GLTFScene;
	class ReflectiveShadowMapPass : public RenderPassBase
	{
	public:
		explicit ReflectiveShadowMapPass() = default;
		explicit ReflectiveShadowMapPass(CommandPoolPtr cmdPool, VkExtent2D resolution);
				~ReflectiveShadowMapPass();

	public:
		ReflectiveShadowMapPass& createAttachments		(void);
		ReflectiveShadowMapPass& createRenderPass		(void);
		ReflectiveShadowMapPass& setDirectionalLight	(std::unique_ptr<DirectionalLight>&& dirLight);
		ReflectiveShadowMapPass& createPipeline			(const DescriptorSetLayoutPtr& globalDescLayout);
		
		void drawGUI(void) override;

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
		SamplerPtr				_rsmSampler				{ nullptr };
		BufferPtr				_viewProjBuffer			{ nullptr };
		DescriptorSetPtr		_descriptorSet			{ nullptr };
		DescriptorPoolPtr		_descriptorPool			{ nullptr };
		DescriptorSetLayoutPtr	_descriptorLayout		{ nullptr };
	};
};

#endif