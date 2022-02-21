// Author : Jihong Shin (snowapril)

#if !defined(VFS_SPECULAR_FILTER_PASS_H)
#define VFS_SPECULAR_FILTER_PASS_H

#include <RenderPass/RenderPassBase.h>

namespace vfs
{
	class SpecularFilterPass : public RenderPassBase
	{
	public:
		explicit SpecularFilterPass(CommandPoolPtr cmdPool,
									VkExtent2D resolution);
				~SpecularFilterPass();

	public:
		SpecularFilterPass& createAttachments	(void);
		SpecularFilterPass& createRenderPass	(void);
		SpecularFilterPass& createFramebuffer	(void);
		SpecularFilterPass& createDescriptors	(void);
		SpecularFilterPass& createPipeline		(void);

		void drawGUI(void) override;
		void processWindowResize(int width, int height) override;
	private:
		void onBeginRenderPass	(const FrameLayout* frameLayout) override;
		void onEndRenderPass	(const FrameLayout* frameLayout) override;
		void onUpdate			(const FrameLayout* frameLayout) override;

		struct SpecularFilterPassDesc
		{
			float	tonemapGamma; 		//  4
			float	tonemapExposure; 	//  8
			int32_t tonemapEnable;		// 12
			int32_t filterMethod;		// 16
		};

	private:
		DescriptorSetPtr			_descriptorSet				{ nullptr };
		DescriptorPoolPtr			_descriptorPool				{ nullptr };
		DescriptorSetLayoutPtr		_descriptorLayout			{ nullptr };
		SamplerPtr					_colorSampler				{ nullptr };
		FramebufferPtr				_framebuffer				{ nullptr };
		VkExtent2D					_resolution					{	0, 0  };

		// Rendering option parameters
		float						_tonemapGamma				{ 2.2f };
		float						_tonemapExposure			{ 0.1f };
		bool						_tonemapEnable				{ false };
		int32_t						_filterMethod				{ 1 };
	};
};

#endif