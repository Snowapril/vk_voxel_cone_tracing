// Author : Jihong Shin (snowapril)

#if !defined(VFS_RADIANCE_INJECTION_PASS_H)
#define VFS_RADIANCE_INJECTION_PASS_H

#include <Common/pch.h>
#include <RenderPass/RenderPassBase.h>

namespace vfs
{
	class DirectionalLight;
	class Voxelizer;

	class RadianceInjectionPass : public RenderPassBase
	{
	public:
		explicit RadianceInjectionPass(DevicePtr device, uint32_t voxelResolution);
				~RadianceInjectionPass();

	public:
		RadianceInjectionPass& initialize			(void);
		RadianceInjectionPass& createDescriptors	(void);
		RadianceInjectionPass& createPipeline		(const DescriptorSetLayoutPtr& globalDescLayout);

		void drawUI(void) override;

	private:
		void onBeginRenderPass	(const FrameLayout* frameLayout) override;
		void onEndRenderPass	(const FrameLayout* frameLayout) override;
		void onUpdate			(const FrameLayout* frameLayout) override;

	private:
		Voxelizer*				_voxelizer				{ nullptr };
		Image*					_voxelRadiance			{ nullptr };
		ImageView*				_voxelRadianceR32View	{ nullptr };
		Sampler*				_voxelSampler			{ nullptr };
		DescriptorPoolPtr		_descriptorPool;
		DescriptorSetPtr		_descriptorSet;
		DescriptorSetLayoutPtr	_descriptorLayout;
		DescriptorPoolPtr		_lightDescriptorPool;
		DescriptorSetPtr		_lightDescriptorSet;
		DescriptorSetLayoutPtr	_lightDescriptorLayout;
		SamplerPtr				_shadowSampler;
		VkSampleCountFlagBits	_sampleCount;
		uint32_t				_voxelResolution;
	};
};

#endif