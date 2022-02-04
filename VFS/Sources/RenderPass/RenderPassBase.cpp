// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <RenderPass/RenderPassBase.h>

namespace vfs
{
	RenderPassBase::RenderPassBase(DevicePtr device)
		: _device(device), _debugUtils(device)
	{
		// Do nothing
	}

	RenderPassBase::RenderPassBase(DevicePtr device,
								   RenderPassPtr renderPass)
		: _device(device), _renderPass(renderPass), _debugUtils(device)
	{
		// Do nothing
	}

	RenderPassBase::~RenderPassBase()
	{
		_renderPassManager = nullptr;
	}

	void RenderPassBase::beginRenderPass(const FrameLayout* frameLayout)
	{
		onBeginRenderPass(frameLayout);
	}

	void RenderPassBase::endRenderPass(const FrameLayout* frameLayout)
	{
		onEndRenderPass(frameLayout);
	}

	void RenderPassBase::update(const FrameLayout* frameLayout)
	{
		onUpdate(frameLayout);
	}

	void RenderPassBase::attachRenderPassManager(RenderPassManager* renderPassManager)
	{
		_renderPassManager = renderPassManager;
	}
};