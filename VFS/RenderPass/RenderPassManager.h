// Author : Jihong Shin (snowapril)

#if !defined(VFS_RENDER_PASS_MANAGER)
#define VFS_RENDER_PASS_MANAGER

#include <pch.h>
#include <unordered_map>
#include <VulkanFramework/DebugUtils.h>

namespace vfs
{
	// Forward declarations
	class RenderPassBase;
	struct FrameLayout;

	//! << NOTICE >>
	//! This class does not manage resource lifetime **except renderpasses**.
	//! You must deallocate heap-allocated resources in separate code.
	class RenderPassManager : NonCopyable
	{
	public:
		explicit RenderPassManager(DevicePtr device);
				~RenderPassManager() = default;

		using ResourcePoolStorage = std::unordered_map<std::string, void*>;
		using RenderPassStorage   = std::unordered_map<std::string, std::unique_ptr<RenderPassBase>>;
	public:
		void registerInputCallbacks	(WindowPtr window);
		void addRenderPass			(std::string name, std::unique_ptr<RenderPassBase>&& renderPass);
		
		// TODO(snowapril) : very dangerous code. must be modified
		RenderPassBase* getRenderPass(const std::string& name);

		// Draw all renderpasses sequentially
		void drawSingleRenderPass	(const std::string& name, const FrameLayout* frameLayout);
		void drawRenderPasses		(const FrameLayout* frameLayout);
		// Draw GUI for all renderpasses sequentially
		void drawGUIRenderPasses		(void);
		void drawDebugInfoRenderPasses	(void);

		template <typename Type>
		Type* get(std::string resourceName)
		{
			ResourcePoolStorage::iterator iter = _resourcePool.find(resourceName);
			assert(iter != _resourcePool.end()); // snowapril : As `get` is developer-level codes, there should not be naming mismatch
			return static_cast<Type*>(iter->second);
		}

		template <typename Type>
		void put(std::string resourceName, Type* resourcePtr)
		{
			_resourcePool.emplace(resourceName, static_cast<void*>(resourcePtr));
		}

	private:
		void processKeyInput(uint32_t key, bool pressed);
		void processCursorPos(double xpos, double ypos);
		void processWindowResize(int width, int height);

	private:
		DevicePtr	_device;
		DebugUtils	_debugUtils;
		ResourcePoolStorage	_resourcePool;
		RenderPassStorage	_renderPasses;
	};
};

#endif