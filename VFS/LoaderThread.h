// Author : Jihong Shin (snowapril)

#if !defined(VFS_LOADER_THREAD_H)
#define VFS_LOADER_THREAD_H

#include <pch.h>
#include <GUI/UIRenderer.h>
#include <future>
#include <thread>

namespace vfs
{
	class SparseVoxelOctree;
	class OctreeBuilder;

	class LoaderThread : NonCopyable
	{
	public:
		explicit LoaderThread(void) = default;
		explicit LoaderThread(vfs::QueuePtr mainQueue, vfs::QueuePtr loaderQueue, 
							  const char* scenePath, const uint32_t octreeLevel);
				~LoaderThread(void);
	
	public:
		void destroyLoaderThread(void);
		void run				(vfs::QueuePtr mainQueue, vfs::QueuePtr loaderQueue,
								 const char* scenePath, const uint32_t octreeLevel);
		bool join				(void);
	
		inline std::shared_ptr<SparseVoxelOctree> getSparseVoxelOctree(void) const
		{
			return _svo;
		}
	private:
		void loaderWorker(const char* scenePath);
	
	private:
		DevicePtr	_device;
		QueuePtr	_graphicsQueue;
		QueuePtr	_loaderQueue;
		uint32_t	_octreeLevel;
		std::shared_ptr<SparseVoxelOctree> _svo;
		std::promise<std::shared_ptr<vfs::OctreeBuilder>>	_builderPromise;
		std::future<std::shared_ptr<vfs::OctreeBuilder>>	_builderFuture;
		std::thread											_loaderThread;
	};
};

#endif