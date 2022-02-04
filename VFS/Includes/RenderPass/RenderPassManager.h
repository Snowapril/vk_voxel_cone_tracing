// Author : Jihong Shin (snowapril)

#if !defined(VFS_RENDER_PASS_MANAGER)
#define VFS_RENDER_PASS_MANAGER

#include <Common/pch.h>
#include <unordered_map>

namespace vfs
{
	class RenderPassBase;

	//! << NOTICE >>
	//! This class does not manage resource lifetime **except renderpasses**.
	//! You must deallocate heap-allocated resources in separate code.
	class RenderPassManager : NonCopyable
	{
	public:
		using ResourcePoolStorage = std::unordered_map<std::string, void*>;

		explicit RenderPassManager(DevicePtr device);
				~RenderPassManager() = default;
	public:
		void registerInputCallbacks	(WindowPtr window);
		void addRenderPass			(std::unique_ptr<RenderPassBase>&& renderPass);
		
		template <typename Type>
		Type* get(std::string resourceName)
		{
			ResourcePoolStorage::iterator iter = _resourcePool.find(resourceName);
			assert(iter != _resourcePool.end()); // Jihong Shin : As `get` is developer-level codes, there should not be naming mismatch
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
		DevicePtr _device;
		ResourcePoolStorage _resourcePool;
		std::vector<std::unique_ptr<RenderPassBase>> _renderPasses;
	};
};

#endif