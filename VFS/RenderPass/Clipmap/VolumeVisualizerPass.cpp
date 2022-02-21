// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <RenderPass/Clipmap/VolumeVisualizerPass.h>

namespace vfs
{
	VolumeVisualizerPass::VolumeVisualizerPass(CommandPoolPtr cmdPool, VkExtent2D resolution)
		: RenderPassBase(cmdPool), _gbufferResolution(resolution)
	{
		// Do nothing
	}

	VolumeVisualizerPass::~VolumeVisualizerPass()
	{

	}

	VolumeVisualizerPass& VolumeVisualizerPass::createAttachments(void)
	{
		return *this;
	}

	VolumeVisualizerPass& VolumeVisualizerPass::createRenderPass(void)
	{
		return *this;
	}

	VolumeVisualizerPass& VolumeVisualizerPass::createFramebuffer(void)
	{
		return *this;
	}

	VolumeVisualizerPass& VolumeVisualizerPass::createPipeline(const DescriptorSetLayoutPtr& globalDescLayout,
		const GLTFScene* scene)
	{
		return *this;
	}

	void VolumeVisualizerPass::onBeginRenderPass(const FrameLayout* frameLayout)
	{
		(void)frameLayout;
	}

	void VolumeVisualizerPass::onEndRenderPass(const FrameLayout* frameLayout)
	{
		(void)frameLayout;
	}

	void VolumeVisualizerPass::onUpdate(const FrameLayout* frameLayout)
	{
		(void)frameLayout;
	}
};