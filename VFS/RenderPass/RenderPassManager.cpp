// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <VulkanFramework/Window.h>
#include <RenderPass/RenderPassManager.h>
#include <RenderPass/RenderPassBase.h>
#include <VulkanFramework/FrameLayout.h>

namespace vfs
{
	RenderPassManager::RenderPassManager(DevicePtr device)
		: _device(device), _debugUtils(device)
	{
		// Do nothing
	}

	void RenderPassManager::registerInputCallbacks(WindowPtr window)
	{
		using namespace std::placeholders;
		Window::KeyCallback			 inputCallback			= std::bind(&RenderPassManager::processKeyInput,	 this, _1, _2);
		Window::CursorPosCallback	 cursorPosCallback		= std::bind(&RenderPassManager::processCursorPos,	 this, _1, _2);
		Window::WindowResizeCallback windowResizeCallback	= std::bind(&RenderPassManager::processWindowResize, this, _1, _2);
		window->operator+=(inputCallback);
		window->operator+=(cursorPosCallback);
		window->operator+=(windowResizeCallback);
	}

	void RenderPassManager::addRenderPass(std::string name, std::unique_ptr<RenderPassBase>&& renderPass)
	{
		// TODO(snowapril) : renderPass->attachRenderPassManager(this);
		_renderPasses.emplace(name, std::move(renderPass));
	}

	RenderPassBase* RenderPassManager::getRenderPass(const std::string& name)
	{
		RenderPassStorage::iterator iter = _renderPasses.find(name);
		assert(iter != _renderPasses.end());
		return (iter->second).get();
	}

	void RenderPassManager::drawSingleRenderPass(const std::string& name, const FrameLayout* frameLayout)
	{
		DebugUtils::ScopedCmdLabel scope = _debugUtils.scopeLabel(frameLayout->commandBuffer, name.c_str());
		getRenderPass(name)->render(frameLayout);
	}

	void RenderPassManager::drawRenderPasses(const FrameLayout* frameLayout)
	{
		for (RenderPassStorage::iterator iter = _renderPasses.begin(); iter != _renderPasses.end(); ++iter)
		{
			DebugUtils::ScopedCmdLabel scope = _debugUtils.scopeLabel(frameLayout->commandBuffer, iter->first.c_str());
			(iter->second)->render(frameLayout);
		}
	}

	void RenderPassManager::drawDebugInfoRenderPasses(void)
	{
		for (RenderPassStorage::iterator iter = _renderPasses.begin(); iter != _renderPasses.end(); ++iter)
		{
			(iter->second)->drawDebugInfo();
		}
	}

	void RenderPassManager::drawGUIRenderPasses(void)
	{
		for (RenderPassStorage::iterator iter = _renderPasses.begin(); iter != _renderPasses.end(); ++iter)
		{
			(iter->second)->drawGUI();
		}
	}

	void RenderPassManager::processKeyInput(uint32_t key, bool pressed)
	{
		for (RenderPassStorage::iterator iter = _renderPasses.begin(); iter != _renderPasses.end(); ++iter)
		{
			(iter->second)->processKeyInput(key, pressed);
		}
	}

	void RenderPassManager::processCursorPos(double xpos, double ypos)
	{
		for (RenderPassStorage::iterator iter = _renderPasses.begin(); iter != _renderPasses.end(); ++iter)
		{
			(iter->second)->processCursorPos(xpos, ypos);
		}
	}

	void RenderPassManager::processWindowResize(int width, int height)
	{
		for (RenderPassStorage::iterator iter = _renderPasses.begin(); iter != _renderPasses.end(); ++iter)
		{
			(iter->second)->processWindowResize(width, height);
		}
	}
};