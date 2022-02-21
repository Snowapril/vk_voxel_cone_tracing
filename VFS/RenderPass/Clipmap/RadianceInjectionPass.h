// Author : Jihong Shin (snowapril)

#if !defined(VFS_RADIANCE_INJECTION_PASS_H)
#define VFS_RADIANCE_INJECTION_PASS_H

#include <pch.h>
#include <RenderPass/RenderPassBase.h>

namespace vfs
{
	class DirectionalLight;
	class Voxelizer;

	class RadianceInjectionPass : public RenderPassBase
	{
	public:
		explicit RadianceInjectionPass(CommandPoolPtr cmdPool, uint32_t voxelResolution);
				~RadianceInjectionPass();

	public:
		RadianceInjectionPass& initialize			(void);
		RadianceInjectionPass& createDescriptors	(void);
		RadianceInjectionPass& createPipeline		(const DescriptorSetLayoutPtr& globalDescLayout);

		void drawGUI(void) override;

	private:
		void onBeginRenderPass	(const FrameLayout* frameLayout) override;
		void onEndRenderPass	(const FrameLayout* frameLayout) override;
		void onUpdate			(const FrameLayout* frameLayout) override;

	private:
		Voxelizer*				_voxelizer				{ nullptr };
		Image*					_voxelRadiance			{ nullptr };
		Image*					_voxelOpacity			{ nullptr };
		ImageView*				_voxelRadianceView		{ nullptr };
		ImageView*				_voxelOpacityView		{ nullptr };
		ImageView*				_voxelRadianceR32View	{ nullptr };
		Sampler*				_voxelSampler			{ nullptr };
		BufferPtr				_possionSampleBuffer	{ nullptr };
		DescriptorPoolPtr		_descriptorPool;
		DescriptorSetPtr		_descriptorSet;
		DescriptorSetLayoutPtr	_descriptorLayout;
		DescriptorPoolPtr		_lightDescriptorPool;
		DescriptorSetPtr		_lightDescriptorSet;
		DescriptorSetLayoutPtr	_lightDescriptorLayout;
		SamplerPtr				_shadowSampler;
		uint32_t				_voxelResolution;
		uint32_t				_frameIndex{ 0 };
	};
};

#endif