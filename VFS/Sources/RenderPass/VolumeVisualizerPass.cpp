// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <RenderPass/VolumeVisualizerPass.h>

namespace vfs
{
	VolumeVisualizerPass::VolumeVisualizerPass(DevicePtr device, VkExtent2D resolution)
		: RenderPassBase(device), _gbufferResolution(resolution)
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