// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <Common/EngineConfig.h>
#include <VulkanFramework/Commands/CommandPool.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Window.h>
#include <VulkanFramework/Queue.h>
#include <Camera.h>
#include <VulkanFramework/RenderPass/RenderPass.h>
#include <VulkanFramework/FrameLayout.h>
#include <RenderPass/GBufferPass.h>
#include <RenderPass/GBufferDebugPass.h>
#include <RenderPass/VoxelizationPass.h>
#include <RenderPass/ShadowMapPass.h>
#include <RenderPass/RenderPassManager.h>
#include <RenderPass/RadianceInjectionPass.h>
#include <RenderPass/VoxelConeTracingPass.h>
#include <RenderPass/DownSampler.h>
#include <RenderPass/Voxelizer.h>
#include <VoxelEngine/OctreeBuilder.h>
#include <VoxelEngine/OctreeVisualizer.h>
#include <VoxelEngine/SparseVoxelizer.h>
#include <VoxelEngine/SparseVoxelOctree.h>
#include <DirectionalLight.h>
#include <VolumeVisualizer.h>
#include <Application.h>
#include <GLTFScene.h>
#include <Renderer.h>
#include <SwapChain.h>
#include <chrono>
#include <algorithm>

namespace vfs
{
	Application::~Application()
    {
    	destroyApplication();
    }
    
    void Application::destroyApplication(void)
    {
        vkDeviceWaitIdle(_device->getDeviceHandle());
        _renderPassManager.reset();
        _gltfScenes.clear();
        _mainCamera.reset();
        _renderer.reset();
        _uiRenderer.reset();
        _mainCommandPool.reset();
        _loaderQueue.reset();
        _presentQueue.reset();
        _graphicsQueue.reset();
        _device.reset();
        _window.reset();
    }
    
    bool Application::initialize(int argc, char* argv[])
    {
        // TODO(snowapril) : use command line arguments
        (void)argc; (void)argv;

        if (!initializeVulkanDevice())
        {
            return false;
        }

        // if (!initializeRenderPassResources())
        // {
        //     return false;
        // }

        _uiRenderer = std::make_unique<UIRenderer>(_window, _device, _graphicsQueue, _renderer->getRenderPass()->getHandle());
        _uiRenderer->createFontTexture(_mainCommandPool);

        _mainCamera = std::make_shared<Camera>(_window, _device, _renderer->getFrameCount());

        _gltfScenes.emplace_back(std::make_shared<GLTFScene>(
            _device, "Scene/Sponza/Sponza.gltf", _graphicsQueue, vfs::VertexFormat::Position3Normal3TexCoord2Tangent4
        ));
    	return true;
    }
    
    void Application::run(void)
    {
        std::shared_ptr<RenderPassManager> renderPassManager = std::make_shared<RenderPassManager>(_device);
        renderPassManager->put("MainCamera", _mainCamera.get());
        renderPassManager->put("MainScenes", &_gltfScenes);

        std::array<BoundingBox<glm::vec3>, DEFAULT_CLIP_REGION_COUNT> clipRegionBoundingBox;
        // TODO(snowapril) : move this bounding box update somewhere
        for (uint32_t clipmapLevel = 0; clipmapLevel < DEFAULT_CLIP_REGION_COUNT; ++clipmapLevel)
        {
            glm::vec3 center = _mainCamera->getOriginPos();
            const float halfSize = static_cast<float>((8) * (1 << clipmapLevel));
            BoundingBox<glm::vec3> bb(center - halfSize, center + halfSize);
            clipRegionBoundingBox[clipmapLevel] = bb;
        }
        renderPassManager->put("ClipRegionBoundingBox", &clipRegionBoundingBox);
        
        vfs::GBufferPass gbufferPass(_device, { 1280, 920 });
        gbufferPass.attachRenderPassManager(renderPassManager.get());
        gbufferPass.createAttachments()
                   .createRenderPass()
                   .createFramebuffer()
                   .createPipeline(_mainCamera->getDescriptorSetLayout(), _gltfScenes[0].get());

        vfs::GBufferDebugPass debugPass(_device, _renderer->getRenderPass());

        _voxelizer = std::make_shared<vfs::Voxelizer>(_device, 128);
        _voxelizer->attachRenderPassManager(renderPassManager.get());
        _voxelizer->createAttachments(_window->getWindowExtent())
                  .createRenderPass()
                  .createFramebuffer(_window->getWindowExtent())
                  .createVoxelClipmap(16)
                  .createDescriptors();
                  
        renderPassManager->put("Voxelizer", _voxelizer.get());

        vfs::VoxelizationPass voxelizationPass(_device, 128, _gltfScenes[0]);
        voxelizationPass.attachRenderPassManager(renderPassManager.get());
        voxelizationPass.initialize()
                        .createVoxelClipmap(16)
                        .createDescriptors()
                        .createPipeline(_mainCamera->getDescriptorSetLayout());

        std::vector<vfs::ImageViewPtr> imageViews;
        for (uint32_t i = 0; i < 4; ++i)
        {
            imageViews.push_back(gbufferPass.getAttachment(i).imageView);
        }

        vfs::CommandBuffer gbufferCmdBuffer(_mainCommandPool->allocateCommandBuffer());

        std::unique_ptr<vfs::DirectionalLight> dirLight = std::make_unique<vfs::DirectionalLight>(_device);
        dirLight->createShadowMap({ 4096, 4096 })
                .setTransform(glm::vec3(0.0f, 15.0f, -5.3f), glm::vec3(0.0f, -1.0f, 0.2f))
                .setColorAndIntensity(glm::vec3(1.0f), 1.0f);

        vfs::ShadowMapPass shadowPass(_device, { 4096, 4096 });
        shadowPass.attachRenderPassManager(renderPassManager.get());
        shadowPass.createRenderPass()
                  .createPipeline(_mainCamera->getDescriptorSetLayout(), _gltfScenes[0].get())
                  .setDirectionalLight(std::move(dirLight));

        vfs::RadianceInjectionPass radianceInjectionPass(_device, 128);
        radianceInjectionPass.attachRenderPassManager(renderPassManager.get());
        radianceInjectionPass.initialize()
            .createDescriptors()
            .createPipeline(_mainCamera->getDescriptorSetLayout());

        
        vfs::VoxelConeTracingPass voxelConeTracingPass(_device, _renderer->getRenderPass(), _window->getWindowExtent());
        voxelConeTracingPass.attachRenderPassManager(renderPassManager.get());
        voxelConeTracingPass.createDescriptors()
                            .createPipeline(_mainCamera->getDescriptorSetLayout());

        vfs::DownSampler downSampler(_device);
        downSampler.createDescriptors()
                   .createPipeline()
                   .createCommandPool(_graphicsQueue);
        renderPassManager->put("DownSampler", &downSampler);

        std::chrono::steady_clock::time_point currentTime = std::chrono::high_resolution_clock::now();

        DebugUtils debugUtils(_device);

        // TODO(snowapril) : move somewhere
        // VkSemaphore voxelizationSemaphore[2];
        // VkSemaphoreCreateInfo semaphoreInfo = {};
        // semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        // vkCreateSemaphore(_device->getDeviceHandle(), &semaphoreInfo, nullptr, &voxelizationSemaphore[0]);
        // vkCreateSemaphore(_device->getDeviceHandle(), &semaphoreInfo, nullptr, &voxelizationSemaphore[1]);
        // 
        // _renderer->addWaitSemaphore(voxelizationSemaphore[0]);
        // _renderer->addSignalSemaphore(voxelizationSemaphore[1]);

        // TODO(snowapril) : move below somewhere

        bool test = true;
        while (!_window->getWindowShouldClose())
        {
            glfwPollEvents();

            std::chrono::steady_clock::time_point nowTime = std::chrono::high_resolution_clock::now();
            const float elapsedTime = std::chrono::duration<float, std::chrono::seconds::period>(
                nowTime - currentTime
                ).count();
            currentTime = nowTime;
            char titleBuf[16];
            snprintf(titleBuf, sizeof(titleBuf), "FPS:%.3f", 1.0f / elapsedTime);
            glfwSetWindowTitle(_window->getWindowHandle(), titleBuf);

            _mainCamera->processInput(_window->getWindowHandle(), elapsedTime);
            _window->processKeyInput();

            {
                vfs::FrameLayout frame = {
                    _mainCamera->getDescriptorSet(_renderer->getCurrentFrameIndex()),
                    gbufferCmdBuffer.getHandle(),
                    _renderer->getCurrentFrameIndex(),
                    elapsedTime,
                };
                _mainCamera->updateCamera(frame.frameIndex);

                // TODO(snowapril) : move this bounding box update somewhere
                for (uint32_t clipmapLevel = 0; clipmapLevel < DEFAULT_CLIP_REGION_COUNT; ++clipmapLevel)
                {
                    glm::vec3 center = _mainCamera->getOriginPos();
                    const float halfSize = static_cast<float>((8) * (1 << clipmapLevel));
                    BoundingBox<glm::vec3> bb(center - halfSize, center + halfSize);
                    clipRegionBoundingBox[clipmapLevel] = bb;
                }

                // 0. GBuffer Pass
                gbufferCmdBuffer.beginRecord(0);
                {
                    DebugUtils::ScopedCmdLabel gbufferPassScope = debugUtils.scopeLabel(gbufferCmdBuffer.getHandle(), "GBufferPass");
                    gbufferPass.beginRenderPass(&frame);
                    for (std::shared_ptr<GLTFScene>& scene : _gltfScenes)
                    {
                        scene->cmdDraw(frame.commandBuffer, gbufferPass.getPipelineLayout(), 0);
                    }
                    gbufferPass.endRenderPass(&frame);
                }

                // 1. Voxelization Pass(Opacity Encoding)
                {
                    DebugUtils::ScopedCmdLabel voxelizationPassScope = debugUtils.scopeLabel(gbufferCmdBuffer.getHandle(), "OpacityEncodingPass");
                    voxelizationPass.beginRenderPass(&frame);
                    voxelizationPass.update(&frame);
                    voxelizationPass.endRenderPass(&frame);
                }

                // 2. Shadow Map Pass
                {
                    DebugUtils::ScopedCmdLabel shadowPassScope = debugUtils.scopeLabel(gbufferCmdBuffer.getHandle(), "ShadowMapPass");
                    shadowPass.beginRenderPass(&frame);
                    shadowPass.update(&frame);
                    shadowPass.endRenderPass(&frame);
                }

                // 3. Radiance Injection Pass (Radiance Encoding)
                {
                    DebugUtils::ScopedCmdLabel radianceInjectionPassScope = debugUtils.scopeLabel(gbufferCmdBuffer.getHandle(), "RadianceInjectionPass");
                    radianceInjectionPass.beginRenderPass(&frame);
                    radianceInjectionPass.update(&frame);
                    radianceInjectionPass.endRenderPass(&frame);
                }

                gbufferCmdBuffer.endRecord();
                vfs::Fence fence(_device, 1, 0);
                _graphicsQueue->submitCmdBuffer({ gbufferCmdBuffer }, &fence);
                fence.waitForAllFences(UINT64_MAX);
            }

            // Main renderer pass for voxel cone tracing and UI Rendering
            {
                VkCommandBuffer cmdBuffer = _renderer->beginFrame();
                vfs::FrameLayout frame = {
                    _mainCamera->getDescriptorSet(_renderer->getCurrentFrameIndex()),
                    cmdBuffer,
                    _renderer->getCurrentFrameIndex(),
                    elapsedTime,
                };
                vfs::CommandBuffer cmd(cmdBuffer);
                _renderer->beginRenderPass(frame.commandBuffer);

                // 4. Voxel Cone Tracing Pass
                {
                    DebugUtils::ScopedCmdLabel vctPassScope = debugUtils.scopeLabel(cmdBuffer, "VoxelConeTracingPass");
                    //visualizer.cmdDraw(&frame);
                    switch (_renderingMode)
                    {
                    case RenderingMode::GI_PASS:
                        voxelConeTracingPass.setRenderingMode(_giRenderingMode);
                        voxelConeTracingPass.update(&frame);
                        break;
                    case RenderingMode::GBUFFER_DEBUG_PASS:
                        debugPass.beginRenderPass(&frame, _mainCamera);
                        // TODO(snowapril) : Quad rendering need
                        debugPass.endRenderPass(&frame);
                        break;
                    case RenderingMode::CLIPMAP_VISUALIZER_PASS:
                        break;
                    default:
                        break;
                    }
                }

                // 5. UI Rendering Pass
                {
                    DebugUtils::ScopedCmdLabel uiPassScope = debugUtils.scopeLabel(cmdBuffer, "UIPass");
                    _uiRenderer->beginUIRender();
                    gbufferPass.drawUI();
                    voxelizationPass.drawUI();
                    _voxelizer->drawUI();
                    shadowPass.drawUI();
                    radianceInjectionPass.drawUI();
                    voxelConeTracingPass.drawUI();
                    _uiRenderer->endUIRender(&frame);
                }
                _renderer->endRenderPass(frame.commandBuffer);
                _renderer->endFrame();
            }
        }
        vkDeviceWaitIdle(_device->getDeviceHandle());
    }

    bool Application::initializeVulkanDevice(void)
    {
        _window = std::make_shared<vfs::Window>(DEFAULT_APP_TITLE, 1280, 920);
        _device = std::make_shared<vfs::Device>(DEFAULT_APP_TITLE);

        VkSurfaceKHR surface = _window->createWindowSurface(_device->getVulkanInstance());

        uint32_t graphicsFamily{ UINT32_MAX }, presentFamily{ UINT32_MAX }, loaderFamily{ UINT32_MAX };
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        _device->getQueueFamilyProperties(&queueFamilyProperties);

        // Find queue family which support both graphics & present
        uint32_t i = 0;
        bool graphicsBits{ false }, presentBits{ false }, loaderBits{ false };
        for (const VkQueueFamilyProperties& queueFamilyProperty : queueFamilyProperties)
        {
            if (queueFamilyProperty.queueCount > 0 &&
                (queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                graphicsFamily = i;
                graphicsBits = true;
            }

            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(_device->getPhysicalDeviceHandle(), i, surface, &presentSupport);
            if (queueFamilyProperty.queueCount > 0 && presentSupport == VK_TRUE)
            {
                presentFamily = i;
                presentBits = true;
            }

            if (graphicsBits && presentBits)
            {
                break;
            }
            else
            {
                ++i;
            }
        }

        i = 0;
        for (const VkQueueFamilyProperties& queueFamilyProperty : queueFamilyProperties)
        {
            if (queueFamilyProperty.queueCount > 0 &&
                (queueFamilyProperty.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                (queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                (queueFamilyProperty.queueFlags & VK_QUEUE_COMPUTE_BIT))
            {
                loaderFamily = i;
                loaderBits = true;
            }

            if (loaderBits && loaderFamily != graphicsFamily) // snowapril : prefer separate family for loader queue
            {
                break;
            }
            else
            {
                ++i;
            }
        }

        bool isAllFamilyFound = ~graphicsFamily && ~presentFamily && ~loaderFamily;
        if (!isAllFamilyFound)
        {
            return false;
        }

        _device->initializeLogicalDevice({ graphicsFamily, presentFamily, loaderFamily });
        _device->initializeMemoryAllocator();

        _graphicsQueue  = std::make_shared<vfs::Queue>(_device, graphicsFamily);
        _presentQueue   = std::make_shared<vfs::Queue>(_device, presentFamily);
        _loaderQueue    = std::make_shared<vfs::Queue>(_device, loaderFamily);

        _mainCommandPool = std::make_shared<vfs::CommandPool>(_device, _graphicsQueue,
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        using namespace std::placeholders;
        Window::KeyCallback inputCallback = std::bind(&Application::processKeyInput, this, _1, _2);
        _window->operator+=(inputCallback);

        _renderer = std::make_unique<Renderer>(
            _device, _mainCommandPool, std::make_unique<SwapChain>(_graphicsQueue, _presentQueue, _window, surface)
            );

        return true;
    }

    bool Application::initializeRenderPassResources(void)
    {
        _renderPassManager = std::make_shared<RenderPassManager>(_device);
        _renderPassManager->put("MainCamera", _mainCamera.get());
        _renderPassManager->put("GLTFScenes", &_gltfScenes);
        
        std::unique_ptr<vfs::GBufferPass> gbufferPass = std::make_unique<vfs::GBufferPass>(_device, _window->getWindowExtent());
        gbufferPass->createAttachments()
                   .createRenderPass()
                   .createFramebuffer()
                   .createPipeline(_mainCamera->getDescriptorSetLayout(), _gltfScenes[0].get());
        _renderPassManager->addRenderPass(std::move(gbufferPass));

        _voxelizer = std::make_shared<vfs::Voxelizer>(_device, 128);
        _voxelizer->createAttachments(_window->getWindowExtent())
                  .createRenderPass()
                  .createFramebuffer(_window->getWindowExtent())
                  .createVoxelClipmap(16)
                  .createDescriptors();
        _renderPassManager->put("Voxelizer", _voxelizer.get());

        std::unique_ptr<vfs::VoxelizationPass> voxelizationPass = std::make_unique<vfs::VoxelizationPass>(_device, 128, _gltfScenes[0]);
        voxelizationPass->createVoxelClipmap(16)
                        .createDescriptors()
                        .createPipeline(_mainCamera->getDescriptorSetLayout());
        _renderPassManager->addRenderPass(std::move(voxelizationPass));

        // TODO(snowapril) : do i need to use unique_ptr for directional light here???
        std::unique_ptr<vfs::DirectionalLight> dirLight = std::make_unique<vfs::DirectionalLight>(_device);
        dirLight->createShadowMap({ 4096, 4096 })
                .setTransform(glm::vec3(0.0f, 16.0f, -5.5f), glm::vec3(0.0f, 15.0f, -5.0f))
                .setColorAndIntensity(glm::vec3(1.0f), 1.0f);

        VkExtent2D shadowResolution = { 4096, 4096 };
        std::unique_ptr<vfs::ShadowMapPass> shadowPass = std::make_unique<vfs::ShadowMapPass>(_device, shadowResolution);
        shadowPass->createRenderPass()
                  .createPipeline(_mainCamera->getDescriptorSetLayout(), _gltfScenes[0].get())
                  .setDirectionalLight(std::move(dirLight));
        _renderPassManager->addRenderPass(std::move(shadowPass));

        std::unique_ptr<vfs::RadianceInjectionPass> radianceInjectionPass = std::make_unique<vfs::RadianceInjectionPass>(_device, 128);
        // TODO(snowapril) : initialize here
        // _renderPassManager->addRenderPass(std::move(radianceInjectionPass));

        std::unique_ptr<vfs::VoxelConeTracingPass> voxelConeTracingPass = std::make_unique<vfs::VoxelConeTracingPass>(
            _device, _renderer->getRenderPass(), _window->getWindowExtent()
        );
        // TODO(snowapril) : initialize here
        // _renderPassManager->addRenderPass(std::move(voxelConeTracingPass));

        // TODO(snowapril) : set semaphore and fence sync here

        return true;
    }

    void Application::processKeyInput(uint32_t key, bool pressed)
    {
        if (pressed)
        {
            if (key == GLFW_KEY_ESCAPE)
            {
                // TODO(snowapril) : move below code to window class
                glfwSetWindowShouldClose(_window->getWindowHandle(), GLFW_TRUE);
            }
            if (key >= GLFW_KEY_1 && key <= GLFW_KEY_8)
            {
                uint32_t keyOffset = key - GLFW_KEY_1;
                _giRenderingMode = static_cast<VoxelConeTracingPass::RenderingMode>(keyOffset);
            }
            // if (key == GLFW_KEY_1)
            // {
            //     _renderingMode = RenderingMode::GI_PASS;
            // }
            // if (key == GLFW_KEY_2)
            // {
            //     _renderingMode = RenderingMode::GBUFFER_DEBUG_PASS;
            // }
            // if (key == GLFW_KEY_3)
            // {
            //     _renderingMode = RenderingMode::CLIPMAP_VISUALIZER_PASS;
            // }
        }
    }
}



/* Sparse voxel octree updating loop
std::shared_ptr<vfs::SparseVoxelizer> voxelizer = std::make_shared<vfs::SparseVoxelizer>();
if (!voxelizer->initialize(_device, _gltfScene, 6))
{
    std::cerr << "[VFS] Failed to initialize voxelizer\n";
    return -1;
}
voxelizer->preVoxelize(_mainCommandPool);

std::shared_ptr<vfs::OctreeBuilder> builder = std::make_shared<vfs::OctreeBuilder>();
if (!builder->initialize(_device, _mainCommandPool, voxelizer))
{
    std::cerr << "[VFS] Failed to initialize OctreeBuilder\n";
    return -1;
}
std::chrono::steady_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
vfs::QueryPool profiler(_device, 2);
std::vector<uint64_t> results(2);

{
    vfs::CommandBuffer voxelCmdBuffer(_mainCommandPool->allocateCommandBuffer());

    voxelCmdBuffer.beginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    profiler.resetQueryPool(voxelCmdBuffer.getHandle());
    voxelizer->cmdVoxelize(voxelCmdBuffer.getHandle());
    profiler.writeTimeStamp(voxelCmdBuffer.getHandle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
    builder->cmdBuild(voxelCmdBuffer.getHandle());
    profiler.writeTimeStamp(voxelCmdBuffer.getHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);
    voxelCmdBuffer.endRecord();

    vfs::FencePtr fence = std::make_shared<vfs::Fence>(_device, 1, 0);
    graphicsQueue->submitCmdBuffer({ voxelCmdBuffer }, fence);
    fence->waitForAllFences(UINT64_MAX);

    profiler.readQueryResults(&results);
    std::cout << static_cast<double>(results[1] - results[0]) * 0.000001 << " (ms)\n";
}
vfs::CameraPtr camera = std::make_shared<vfs::Camera>();
camera->initialize(window, _device, , vfs::DEFAULT_NUM_FRAMES);

vfs::GBufferPass gbufferPass;
if (!gbufferPass.initialize(_device, { 1280, 920 }, camera->getDescriptorSetLayout()))
{
    std::cerr << "[VFS] Failed to initialize GBuffer\n";
    return -1;
}

vfs::GLTFScene gltfScene;
if (!gltfScene.initialize(_device, "Scene/Sponza/Sponza.gltf", loaderQueue, vfs::VertexFormat::Position3Normal3TexCoord2Tangent4))
{
    std::cerr << "[VFS] Failed to load gltf scene\n";
    return -1;
}

gltfScene.allocateDescriptorSet(gbufferPass.createDescriptorSet());

vfs::OctreeVisualizer visualizer(_device, camera, builder, renderer.getRenderPass()->getHandle());
visualizer.buildPointClouds(_mainCommandPool);

while (!window->getWindowShouldClose())
{
    glfwPollEvents();

    std::chrono::steady_clock::time_point nowTime = std::chrono::high_resolution_clock::now();
    const float elapsedTime = std::chrono::duration<float, std::chrono::seconds::period>(
        nowTime - currentTime
    ).count();
    currentTime = nowTime;

    camera->processInput(window->getWindowHandle(), elapsedTime);
    {
        VkCommandBuffer cmdBuffer = renderer.beginFrame();
        vfs::FrameLayout frame = {
            camera->getDescriptorSet(renderer.getCurrentFrameIndex()),
            cmdBuffer,
            renderer.getCurrentFrameIndex(),
            elapsedTime,
        };
        vfs::CommandBuffer cmd(cmdBuffer);
        camera->updateCamera(frame.frameIndex);

        gbufferPass.beginRenderPass(&frame, camera);
        gltfScene.cmdDraw(frame.commandBuffer, gbufferPass.getPipelineLayout(), 0);
        gbufferPass.endRenderPass(&frame);

        uiRenderer.beginUIRender();
        renderer.beginRenderPass(frame.commandBuffer);
        visualizer.cmdDraw(&frame);
        uiRenderer.endUIRender(&frame);
        renderer.endRenderPass(frame.commandBuffer);
        renderer.endFrame();
    }
}
*/