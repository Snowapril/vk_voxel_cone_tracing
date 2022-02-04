// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <VulkanFramework/Window.h>
#include <RenderPass/RenderPassManager.h>
#include <RenderPass/RenderPassBase.h>

namespace vfs
{
	RenderPassManager::RenderPassManager(DevicePtr device)
		: _device(device)
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

	void RenderPassManager::addRenderPass(std::unique_ptr<RenderPassBase>&& renderPass)
	{
		renderPass->attachRenderPassManager(this);
		_renderPasses.emplace_back(std::move(renderPass));
	}

	void RenderPassManager::processKeyInput(uint32_t key, bool pressed)
	{
		for (std::unique_ptr<RenderPassBase>& renderPass : _renderPasses)
		{
			renderPass->processKeyInput(key, pressed);
		}
	}

	void RenderPassManager::processCursorPos(double xpos, double ypos)
	{
		for (std::unique_ptr<RenderPassBase>& renderPass : _renderPasses)
		{
			renderPass->processCursorPos(xpos, ypos);
		}
	}

	void RenderPassManager::processWindowResize(int width, int height)
	{
		for (std::unique_ptr<RenderPassBase>& renderPass : _renderPasses)
		{
			renderPass->processWindowResize(width, height);
		}
	}
};