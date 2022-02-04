// Author : Jihong Shin (snowapril)

#if !defined(VFS_GBUFFER_DEBUG_PASS_H)
#define VFS_GBUFFER_DEBUG_PASS_H

#include <Common/pch.h>
#include <Common/VertexFormat.h>

namespace vfs
{
	struct FrameLayout;

	class GBufferDebugPass : NonCopyable
	{
	public:
		explicit GBufferDebugPass() = default;
		explicit GBufferDebugPass(vfs::DevicePtr device, const RenderPassPtr& renderPass);
				~GBufferDebugPass();

	public:
		bool				initialize			(vfs::DevicePtr device, const RenderPassPtr& renderPass);
		DescriptorSetPtr	createDescriptorSet	(void);
		void				beginRenderPass		(const FrameLayout* frameLayout, const CameraPtr& camera);
		void				endRenderPass		(const FrameLayout* frameLayout);

		inline PipelineLayoutPtr getPipelineLayout(void) const
		{
			return _pipelineLayout;
		}
	private:
		bool initializePipeline(const RenderPassPtr& renderPass);

	private:
		DevicePtr					_device;
		GraphicsPipelinePtr			_pipeline;
		PipelineLayoutPtr			_pipelineLayout;
		DescriptorPoolPtr			_descriptorPool;
		DescriptorSetLayoutPtr		_descriptorLayout;
		VkSampleCountFlagBits		_sampleCounts		{ VK_SAMPLE_COUNT_1_BIT };
	};
};

#endif