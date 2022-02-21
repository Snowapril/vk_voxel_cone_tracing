// Author : Jihong Shin (snowapril)

#if !defined(VFS_APPLICATION_H)
#define VFS_APPLICATION_H

#include <pch.h>
#include <GUI/UIRenderer.h>
#include <Renderer.h>
#include <RenderPass/RenderPassBase.h>
#include <RenderPass/Clipmap/VoxelConeTracingPass.h>
#include <SceneManager.h>

#include <RenderPass/Clipmap/BorderWrapper.h>
#include <RenderPass/Clipmap/CopyAlpha.h>
#include <RenderPass/Clipmap/DownSampler.h>
#include <RenderPass/Clipmap/ClipmapCleaner.h>

namespace vfs
{
	class GLTFScene;
	class SceneManager;
	class Voxelizer;

	enum class VCTMethod : unsigned char
	{
		ClipmapMethod	= 0,
		OctreeMethod	= 1,
		Last			= 2
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
		bool initializeVulkanDevice		(void);
		bool buildCommonPasses			(void);
		bool buildClipmapMethodPasses	(void);
		bool buildSVOMethodPasses		(void);

		void processKeyInput			(uint32_t key, bool pressed);
		void updateClipRegionBoundingBox(void);

	private:
		WindowPtr		_window;
		DevicePtr		_device;
		QueuePtr		_graphicsQueue;
		QueuePtr		_presentQueue;
		QueuePtr		_loaderQueue;
		CommandPoolPtr	_mainCommandPool;
		CameraPtr		_mainCamera;
		std::unique_ptr<UIRenderer>				_uiRenderer;
		std::unique_ptr<Renderer>				_renderer;
		std::unique_ptr<SceneManager>			_sceneManager;
		std::shared_ptr<RenderPassManager>		_renderPassManager;
		std::shared_ptr<Voxelizer>				_voxelizer;

		std::unique_ptr<DownSampler>	_clipmapDownSampler;
		std::unique_ptr<BorderWrapper>	_clipmapBorderWrapper;
		std::unique_ptr<ClipmapCleaner> _clipmapCleaner;
		std::unique_ptr<CopyAlpha>		_clipmapCopyAlpha;

		VCTMethod _vctMethod{ VCTMethod::ClipmapMethod };
	};
};

#endif