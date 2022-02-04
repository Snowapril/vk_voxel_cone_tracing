// Author : Jihong Shin (snowapril)

#if !defined(VFS_APPLICATION_H)
#define VFS_APPLICATION_H

#include <Common/pch.h>
#include <UIEngine/UIRenderer.h>
#include <Renderer.h>
#include <RenderPass/RenderPassBase.h>
#include <RenderPass/VoxelConeTracingPass.h>

namespace vfs
{
	class GLTFScene;
	class Voxelizer;

	enum class RenderingMode : uint16_t
	{
		GI_PASS					= 0,
		GBUFFER_DEBUG_PASS		= 1,
		CLIPMAP_VISUALIZER_PASS = 2,
	};

	class Application : vfs::NonCopyable
	{
	public:
		explicit Application() = default;
				~Application();
	
	public:
		void destroyApplication	(void);
		bool initialize			(int argc, char* argv[]);
		void run				(void);

	private:
		bool initializeVulkanDevice			(void);
		bool initializeRenderPassResources	(void);

		void processKeyInput(uint32_t key, bool pressed);

	private:
		WindowPtr		_window;
		DevicePtr		_device;
		QueuePtr		_graphicsQueue;
		QueuePtr		_presentQueue;
		QueuePtr		_loaderQueue;
		CommandPoolPtr	_mainCommandPool;
		CameraPtr		_mainCamera;
		std::unique_ptr<UIRenderer> _uiRenderer;
		std::unique_ptr<Renderer> _renderer;
		std::vector<std::shared_ptr<GLTFScene>> _gltfScenes;
		std::shared_ptr<RenderPassManager> _renderPassManager;
		std::shared_ptr<Voxelizer> _voxelizer;
		RenderingMode _renderingMode{ RenderingMode::GI_PASS };
		VoxelConeTracingPass::RenderingMode _giRenderingMode{ VoxelConeTracingPass::RenderingMode::CombinedGI };
	};
};

#endif