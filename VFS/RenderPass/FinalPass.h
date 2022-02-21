// Author : Jihong Shin (snowapril)

#if !defined(VFS_FINAL_PASS_H)
#define VFS_FINAL_PASS_H

#include <RenderPass/RenderPassBase.h>

namespace vfs
{
	class FinalPass : public RenderPassBase
	{
	public:
		explicit FinalPass(CommandPoolPtr cmdPool,
						   RenderPassPtr renderPass);
				~FinalPass();

	public:
		FinalPass& createDescriptors	(void);
		FinalPass& createPipeline		(void);

	private:
		void onUpdate			(const FrameLayout* frameLayout) override;

	private:
		DescriptorSetPtr			_descriptorSet				{ nullptr };
		DescriptorPoolPtr			_descriptorPool				{ nullptr };
		DescriptorSetLayoutPtr		_descriptorLayout			{ nullptr };
	};
};

#endif