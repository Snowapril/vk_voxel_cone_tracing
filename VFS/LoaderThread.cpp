// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <LoaderThread.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/Commands/CommandPool.h>
#include <VulkanFramework/Commands/CommandBuffer.h>
#include <VulkanFramework/QueryPool.h>
#include <VulkanFramework/Sync/Fence.h>
#include <RenderPass/Octree/SparseVoxelizer.h>
#include <RenderPass/Octree/OctreeBuilder.h>
#include <GLTFScene.h>

namespace vfs
{
	LoaderThread::LoaderThread(vfs::QueuePtr mainQueue, vfs::QueuePtr loaderQueue, 
                               const char* scenePath, const uint32_t octreeLevel)
    {
        run(mainQueue, loaderQueue, scenePath, octreeLevel);
    }
    
    LoaderThread::~LoaderThread(void)
    {
    	destroyLoaderThread();
    }
    
    void LoaderThread::destroyLoaderThread(void)
    {
    	_svo.reset();
    	_loaderQueue.reset();
    	_graphicsQueue.reset();
    	_device.reset();
    }
    
    void LoaderThread::run(vfs::QueuePtr mainQueue, vfs::QueuePtr loaderQueue,
                           const char* scenePath, const uint32_t octreeLevel)
    {
    	_device			= mainQueue->getDevicePtr();
    	_graphicsQueue	= mainQueue;
    	_loaderQueue	= loaderQueue;
        _octreeLevel    = octreeLevel;
    
        _builderPromise = std::promise<std::shared_ptr<vfs::OctreeBuilder>>();
        _builderFuture = _builderPromise.get_future();
        _loaderThread = std::thread(&LoaderThread::loaderWorker, this, scenePath);
    }
    
    bool LoaderThread::join(void)
    {
        if (_builderFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        {
            return false;
        }
    
        _loaderThread.join();
        std::shared_ptr<vfs::OctreeBuilder> builder = _builderFuture.get();
        // TODO(snowapril) : pass this to _svo
        return true;
    }
    
    void LoaderThread::loaderWorker(const char* scenePath)
    {
        vfs::CommandPoolPtr loaderCmdPool = std::make_shared<vfs::CommandPool>(_device, _loaderQueue,
            VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    
        std::shared_ptr<vfs::GLTFScene> scene = std::make_shared<vfs::GLTFScene>(_device, scenePath, _loaderQueue, 
                                                                 vfs::VertexFormat::Position3Normal3TexCoord2Tangent4);
    
        // TODO(snowapril) : replace below nullptr to valid light
        // std::shared_ptr<vfs::SparseVoxelizer> voxelizer = std::make_shared<vfs::SparseVoxelizer>(_device, scene, nullptr, _octreeLevel);
        // voxelizer->preVoxelize(loaderCmdPool);
        // 
        // std::shared_ptr<vfs::OctreeBuilder> builder = std::make_shared<vfs::OctreeBuilder>(_device, loaderCmdPool, voxelizer);
        // 
        // vfs::QueryPool profiler(_device, 4);
        // {
        //     vfs::CommandBuffer cmdBuffer(loaderCmdPool->allocateCommandBuffer());
        //     cmdBuffer.beginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        // 
        //     profiler.resetQueryPool(cmdBuffer.getHandle());
        // 
        //     profiler.writeTimeStamp(cmdBuffer.getHandle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
        //     voxelizer->cmdVoxelize(cmdBuffer.getHandle());
        //     profiler.writeTimeStamp(cmdBuffer.getHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);
        // 
        //     profiler.writeTimeStamp(cmdBuffer.getHandle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 2);
        //     builder->cmdBuild(cmdBuffer.getHandle());
        //     profiler.writeTimeStamp(cmdBuffer.getHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 3);
        // 
        //     cmdBuffer.endRecord();
        // 
        //     vfs::Fence fence(_device, 1, 0);
        //     _loaderQueue->submitCmdBuffer({ cmdBuffer }, &fence);
        //     fence.waitForAllFences(UINT64_MAX);
        // }
        // 
        // std::vector<uint64_t> results;
        // profiler.readQueryResults(&results);
        // 
        // VFS_INFO << "Total Voxelization time : " << static_cast<double>(results[3] - results[0]) * 0.000001 << " (ms)";
        // VFS_INFO << "Voxel fragment list building time : " << static_cast<double>(results[3] - results[2]) * 0.000001 << " (ms)";
        // VFS_INFO << "Sparse voxel octree building time : " << static_cast<double>(results[1] - results[0]) * 0.000001 << " (ms)";
        // 
        // // TODO(snowapril) : error handling
        // _builderPromise.set_value(builder);
    }
};