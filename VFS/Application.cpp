// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <Util/EngineConfig.h>
#include <Common/CPUTimer.h>
#include <Common/Logger.h>
#include <VulkanFramework/Commands/CommandPool.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Window.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/RenderPass/RenderPass.h>
#include <VulkanFramework/FrameLayout.h>
#include <RenderPass/GBufferPass.h>
#include <RenderPass/Clipmap/VoxelizationPass.h>
#include <RenderPass/ShadowMapPass.h>
#include <RenderPass/ReflectiveShadowMapPass.h>
#include <RenderPass/RenderPassManager.h>
#include <RenderPass/Clipmap/RadianceInjectionPass.h>
#include <RenderPass/Clipmap/VoxelConeTracingPass.h>
#include <RenderPass/Clipmap/Voxelizer.h>
#include <RenderPass/SpecularFilterPass.h>
#include <RenderPass/FinalPass.h>
#include <RenderPass/Octree/OctreeBuilder.h>
#include <RenderPass/Octree/OctreeVisualizer.h>
#include <RenderPass/Octree/SparseVoxelizer.h>
#include <RenderPass/Octree/OctreeVoxelConeTracing.h>
#include <RenderPass/Octree/SparseVoxelOctree.h>
#include <VulkanFramework/QueryPool.h>
#include <GUI/ImGuiUtil.h>
#include <imgui/imgui.h>
#include <DirectionalLight.h>

#include <RenderPass/Clipmap/VolumeVisualizer.h>
#include <Camera.h>
#include <Application.h>
#include <Renderer.h>
#include <SwapChain.h>

#include <chrono>

namespace vfs
{
	Application::~Application()
    {
    	destroyApplication();
    }
    
    void Application::destroyApplication(void)
    {
        vkDeviceWaitIdle(_device->getDeviceHandle());
        _clipmapDownSampler.reset();
        _clipmapBorderWrapper.reset();
        _clipmapCleaner.reset();
        _clipmapCopyAlpha.reset();
        _renderPassManager.reset();
        _sceneManager.reset();
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
        (void)argc; (void)argv; // TODO(snowapril) : support CLI arguments

        if (!initializeVulkanDevice())
        {
            VFS_ERROR << "Failed to initialize vulkan device";
            return false;
        }

        _mainCamera     = std::make_shared<Camera>(_window, _device, _renderer->getFrameCount());
        _sceneManager   = std::make_unique<SceneManager>(_graphicsQueue, VertexFormat::Position3Normal3TexCoord2Tangent4);
        _uiRenderer     = std::make_unique<UIRenderer>(_window, _device, _graphicsQueue, _renderer->getSwapChainRenderPass()->getHandle());
        _uiRenderer->createFontTexture(_mainCommandPool);

        _renderPassManager = std::make_shared<RenderPassManager>(_device);
        _renderPassManager->put("MainCamera",   _mainCamera.get());
        _renderPassManager->put("SceneManager", _sceneManager.get());

        if (!buildCommonPasses())
        {
            VFS_ERROR << "Failed to build common passes";
            return false;
        }

        switch (_vctMethod)
        {
        case VCTMethod::ClipmapMethod:
            if (!buildClipmapMethodPasses())
            {
                VFS_ERROR << "Failed to build clipmap method passes";
                return false;
            }
            break;
        case VCTMethod::OctreeMethod: // Not yet work
            if (!buildClipmapMethodPasses())
            {
                VFS_ERROR << "Failed to build sparse voxel octree method passes";
                return false;
            }
            break;
        default:
            assert(static_cast<unsigned char>(_vctMethod) <= static_cast<unsigned char>(VCTMethod::Last));
        }

    	return true;
    }

    void Application::updateClipRegionBoundingBox(void)
    {
        std::array<BoundingBox<glm::vec3>, DEFAULT_CLIP_REGION_COUNT>* clipRegionBBox =
            _renderPassManager->get<std::array<BoundingBox<glm::vec3>, DEFAULT_CLIP_REGION_COUNT>>("ClipRegionBoundingBox");

        for (uint32_t clipmapLevel = 0; clipmapLevel < DEFAULT_CLIP_REGION_COUNT; ++clipmapLevel)
        {
            glm::vec3 center = _mainCamera->getOriginPos();
            const float halfSize = static_cast<float>((DEFAULT_VOXEL_EXTENT_L0 >> 1) * (1 << clipmapLevel));
            BoundingBox<glm::vec3> bb(center - halfSize, center + halfSize);
            clipRegionBBox->at(clipmapLevel) = bb;
        }
    }

    
    void Application::run(void)
    {
        QueryPool preQueryPool(_device, 8);
        std::vector<VkDeviceSize> preQueryResults(8);
        QueryPool mainQueryPool(_device, 4);
        std::vector<VkDeviceSize> mainQueryResults(4);
        Fence fence(_device, 1, 0);
        DebugUtils debugUtils(_device);

        CommandBuffer preCmdBuffer(_mainCommandPool->allocateCommandBuffer());
        std::chrono::steady_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
        while (!_window->getWindowShouldClose())
        {
            glfwPollEvents();

            std::chrono::steady_clock::time_point nowTime = std::chrono::high_resolution_clock::now();
            const float elapsedTime = std::chrono::duration<float, std::chrono::seconds::period>(
                nowTime - currentTime
            ).count();
            currentTime = nowTime;

            _mainCamera->processInput(_window->getWindowHandle(), elapsedTime);
            _window->processKeyInput();
            updateClipRegionBoundingBox();

            {
                vfs::FrameLayout frame = {
                    _mainCamera->getDescriptorSet(_renderer->getCurrentFrameIndex()),
                    preCmdBuffer.getHandle(),
                    _renderer->getCurrentFrameIndex(),
                    elapsedTime,
                };
                _mainCamera->updateCamera(frame.frameIndex);
            
                preCmdBuffer.beginRecord(0);
                preQueryPool.resetQueryPool(preCmdBuffer.getHandle());
            
                // 0. GBuffer Pass
                {
                    preQueryPool.writeTimeStamp(preCmdBuffer.getHandle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
                    _renderPassManager->drawSingleRenderPass("GBuffer", &frame);
                    preQueryPool.writeTimeStamp(preCmdBuffer.getHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);
                }
            
                // 1. Voxelization Pass(Opacity Encoding)
                {
                    preQueryPool.writeTimeStamp(preCmdBuffer.getHandle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 2);
                    _renderPassManager->drawSingleRenderPass("VoxelizationPass", &frame);
                    preQueryPool.writeTimeStamp(preCmdBuffer.getHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 3);
                }
            
                // 2. Shadow Map Pass
                {
                    preQueryPool.writeTimeStamp(preCmdBuffer.getHandle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 4);
                    _renderPassManager->drawSingleRenderPass("RSMPass", &frame);
                    preQueryPool.writeTimeStamp(preCmdBuffer.getHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 5);
                }
            
                // 3. Radiance Injection Pass (Radiance Encoding)
                {
                    preQueryPool.writeTimeStamp(preCmdBuffer.getHandle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 6);
                    _renderPassManager->drawSingleRenderPass("RadianceInjectionPass", &frame);
                    preQueryPool.writeTimeStamp(preCmdBuffer.getHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 7);
                }
            
                preCmdBuffer.endRecord();
                
                // TODO(snowapril) : sync using semaphore
                fence.resetFence();
                _graphicsQueue->submitCmdBuffer({ preCmdBuffer }, &fence);
                fence.waitForAllFences(UINT64_MAX);
            
                preQueryPool.readQueryResults(&preQueryResults);
            }

            // Main renderer pass for voxel cone tracing and UI Rendering
            {
                CommandBuffer cmdBuffer(_renderer->beginFrame());
                mainQueryPool.resetQueryPool(cmdBuffer.getHandle());

                vfs::FrameLayout frame = {
                    _mainCamera->getDescriptorSet(_renderer->getCurrentFrameIndex()),
                    cmdBuffer.getHandle(),
                    _renderer->getCurrentFrameIndex(),
                    elapsedTime,
                };

                // 4. Voxel Cone Tracing Pass
                {
                    mainQueryPool.writeTimeStamp(cmdBuffer.getHandle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
                    _renderPassManager->drawSingleRenderPass("VoxelConeTracingPass", &frame);
                    mainQueryPool.writeTimeStamp(cmdBuffer.getHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);
                }

                // 5. Specular filtering pass
                {
                    mainQueryPool.writeTimeStamp(cmdBuffer.getHandle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 2);
                    _renderPassManager->drawSingleRenderPass("SpecularFilterPass", &frame);
                    mainQueryPool.writeTimeStamp(cmdBuffer.getHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 3);
                }

                _renderer->beginRenderPass(frame.commandBuffer);
                _renderPassManager->drawSingleRenderPass("FinalPass", &frame);

                // 6. UI Rendering Pass
                {
                    DebugUtils::ScopedCmdLabel uiPassScope = debugUtils.scopeLabel(cmdBuffer.getHandle(), "UIPass");
                    _uiRenderer->beginUIRender();
                    _renderPassManager->drawGUIRenderPasses();
                    _sceneManager->drawGUI();

                    if (ImGui::TreeNode("Performance Metrices"))
                    {
                        const float gbufferPassMs             = static_cast<float>(preQueryResults[1] - preQueryResults[0]) * 0.000001f;
                        const float voxelizationPassMs        = static_cast<float>(preQueryResults[3] - preQueryResults[2]) * 0.000001f;
                        const float shadowPassMs              = static_cast<float>(preQueryResults[5] - preQueryResults[4]) * 0.000001f;
                        const float radianceInjectionPassMs   = static_cast<float>(preQueryResults[7] - preQueryResults[6]) * 0.000001f;
                        const float voxelConeTracingPassMs    = static_cast<float>(mainQueryResults[1] - mainQueryResults[0]) * 0.000001f;
                        const float specularFilterPassMs      = static_cast<float>(mainQueryResults[3] - mainQueryResults[2]) * 0.000001f;
                        const float totalMs = gbufferPassMs + voxelizationPassMs + shadowPassMs +
                                              radianceInjectionPassMs + voxelConeTracingPassMs + specularFilterPassMs;

                        ImGui::PlotVar("GBuffer Pass",              gbufferPassMs);
                        ImGui::PlotVar("Voxelization Pass",         voxelizationPassMs);
                        ImGui::PlotVar("Shadow Pass",               shadowPassMs);
                        ImGui::PlotVar("Radiance Injection Pass",   radianceInjectionPassMs);
                        ImGui::PlotVar("Voxel Cone Tracing Pass",   voxelConeTracingPassMs);
                        ImGui::PlotVar("Specular Filter Pass",      specularFilterPassMs);
                        ImGui::PlotVar("Total",                     totalMs);
                        ImGui::TreePop();
                    }

                    if (ImGui::Begin("DebugInfo"))
                    {
                        _renderPassManager->drawDebugInfoRenderPasses();
                    }
                    ImGui::End();

                    _uiRenderer->endUIRender(&frame);
                }
                _renderer->endRenderPass(cmdBuffer.getHandle());
                _renderer->endFrame();
                mainQueryPool.readQueryResults(&mainQueryResults);
            }
        }
        vkDeviceWaitIdle(_device->getDeviceHandle());
    }

    bool Application::initializeVulkanDevice(void)
    {
        _window = std::make_shared<vfs::Window>(DEFAULT_APP_TITLE, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
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

    bool Application::buildCommonPasses(void)
    {
        {
            std::unique_ptr<GBufferPass> gbufferPass = std::make_unique<GBufferPass>(_mainCommandPool, _window->getWindowExtent());
            CPUTimer timer;
            gbufferPass->attachRenderPassManager(_renderPassManager.get());
            gbufferPass->createAttachments()
                       .createRenderPass()
                       .createFramebuffer()
                       .createPipeline(_mainCamera->getDescriptorSetLayout());
            gbufferPass->initializeDebugPass();
            VFS_INFO << "GBuffer pass loaded ( " << timer.elapsedSeconds() << " second )";
            _renderPassManager->addRenderPass("GBuffer", std::move(gbufferPass));
        }

        {
            const VkExtent2D shadowResolution = { 4096, 4096 };
            std::unique_ptr<ReflectiveShadowMapPass> rsmPass = std::make_unique<ReflectiveShadowMapPass>(_mainCommandPool, shadowResolution);
            CPUTimer timer;
            std::unique_ptr<vfs::DirectionalLight> dirLight = std::make_unique<vfs::DirectionalLight>(_device);
            dirLight->createShadowMap({ 4096, 4096 })
                    .setTransform(glm::vec3(0.0f, 30.0f, -5.3f), glm::vec3(0.0f, -1.0f, 0.2f))
                    .setColorAndIntensity(glm::vec3(1.0f), 1.0f);

            rsmPass->attachRenderPassManager(_renderPassManager.get());
            rsmPass->createAttachments()
                   .createRenderPass()
                   .createPipeline(_mainCamera->getDescriptorSetLayout())
                   .setDirectionalLight(std::move(dirLight));
            VFS_INFO << "Reflective shadow map pass loaded ( " << timer.elapsedSeconds() << " second )";
            _renderPassManager->addRenderPass("RSMPass", std::move(rsmPass));
        }

        return true;
    }

    bool Application::buildClipmapMethodPasses(void)
    {
        std::array<BoundingBox<glm::vec3>, DEFAULT_CLIP_REGION_COUNT> clipRegionBoundingBox;
        _renderPassManager->put("ClipRegionBoundingBox", &clipRegionBoundingBox);
        updateClipRegionBoundingBox();
        
        {
            _voxelizer = std::make_unique<vfs::Voxelizer>(_mainCommandPool, DEFAULT_VOXEL_RESOLUTION);
            CPUTimer timer;
            _voxelizer->attachRenderPassManager(_renderPassManager.get());
            _voxelizer->createAttachments(_window->getWindowExtent())
                       .createRenderPass()
                       .createFramebuffer(_window->getWindowExtent())
                       .createVoxelClipmap()
                       .createDescriptors();
            VFS_INFO << "Voxelizer loaded ( " << timer.elapsedSeconds() << " second )";
            _renderPassManager->put("Voxelizer", _voxelizer.get());
        }

        {
            std::unique_ptr<VoxelizationPass> voxelizationPass = std::make_unique<VoxelizationPass>(_mainCommandPool, DEFAULT_VOXEL_RESOLUTION);
            CPUTimer timer;
            voxelizationPass->attachRenderPassManager(_renderPassManager.get());
            voxelizationPass->initialize(DEFAULT_VOXEL_RESOLUTION, DEFAULT_VOXEL_EXTENT_L0, _mainCamera->getDescriptorSetLayout());
            //voxelizationPass->createOpacityVoxelSlice();
            VFS_INFO << "Opacity Voxelization pass loaded ( " << timer.elapsedSeconds() << " second )";
            _renderPassManager->addRenderPass("VoxelizationPass", std::move(voxelizationPass));
        }

        {
            std::unique_ptr<RadianceInjectionPass> radianceInjectionPass = std::make_unique<RadianceInjectionPass>(_mainCommandPool, DEFAULT_VOXEL_RESOLUTION);
            CPUTimer timer;
            radianceInjectionPass->attachRenderPassManager(_renderPassManager.get());
            radianceInjectionPass->initialize()
                                  .createDescriptors()
                                  .createPipeline(_mainCamera->getDescriptorSetLayout());
            VFS_INFO << "Radiance injection pass loaded ( " << timer.elapsedSeconds() << " second )";
            _renderPassManager->addRenderPass("RadianceInjectionPass", std::move(radianceInjectionPass));
        }
        
        {
            std::unique_ptr<VoxelConeTracingPass> voxelConeTracingPass = std::make_unique<VoxelConeTracingPass>(_mainCommandPool, _window->getWindowExtent());
            CPUTimer timer;
            voxelConeTracingPass->attachRenderPassManager(_renderPassManager.get());
            voxelConeTracingPass->createAttachments()
                                .createRenderPass()
                                .createFramebuffer()
                                .createDescriptors()
                                .createPipeline(_mainCamera->getDescriptorSetLayout());
            VFS_INFO << "Voxel cone tracing GI pass loaded ( " << timer.elapsedSeconds() << " second )";
            _renderPassManager->addRenderPass("VoxelConeTracingPass", std::move(voxelConeTracingPass));
        }

        {
            std::unique_ptr<SpecularFilterPass> specularFilterPass = std::make_unique<SpecularFilterPass>(_mainCommandPool, _window->getWindowExtent());
            CPUTimer timer;
            specularFilterPass->attachRenderPassManager(_renderPassManager.get());
            specularFilterPass->createAttachments()
                              .createRenderPass()
                              .createFramebuffer()
                              .createDescriptors()
                              .createPipeline();
            VFS_INFO << "Specular filter pass loaded ( " << timer.elapsedSeconds() << " second )";
            _renderPassManager->addRenderPass("SpecularFilterPass", std::move(specularFilterPass));
        }

        {
            std::unique_ptr<FinalPass> finalPass = std::make_unique<FinalPass>(_mainCommandPool, _renderer->getSwapChainRenderPass());
            CPUTimer timer;
            finalPass->attachRenderPassManager(_renderPassManager.get());
            finalPass->createDescriptors()
                     .createPipeline();
            VFS_INFO << "Final pass loaded ( " << timer.elapsedSeconds() << " second )";
            _renderPassManager->addRenderPass("FinalPass", std::move(finalPass));
        }

        const ImageView* voxelOpacityView   = _renderPassManager->get<ImageView>("VoxelOpacityView");
        const ImageView* voxelRadianceView  = _renderPassManager->get<ImageView>("VoxelRadianceView");
        const Sampler*   voxelSampler       = _renderPassManager->get<Sampler>("VoxelSampler");

        _clipmapDownSampler = std::make_unique<DownSampler>(_device);
        {
            CPUTimer timer;
            _clipmapDownSampler->createDescriptors(voxelOpacityView, voxelRadianceView, voxelSampler)
                                .createPipeline();
            VFS_INFO << "Downsampler loaded ( " << timer.elapsedSeconds() << " second )";
        }
        _renderPassManager->put("DownSampler", _clipmapDownSampler.get());

        _clipmapBorderWrapper = std::make_unique<BorderWrapper>(_device);
        {
            CPUTimer timer;
            _clipmapBorderWrapper->createDescriptors(voxelOpacityView, voxelRadianceView, voxelSampler, _mainCommandPool)
                                  .createPipeline();
            VFS_INFO << "BorderWrapper loaded ( " << timer.elapsedSeconds() << " second )";
        }
        _renderPassManager->put("BorderWrapper", _clipmapBorderWrapper.get());

        _clipmapCleaner = std::make_unique<ClipmapCleaner>(_device);
        {
            CPUTimer timer;
            _clipmapCleaner->createDescriptors(voxelOpacityView, voxelRadianceView, voxelSampler)
                           .createPipeline();
            VFS_INFO << "ClipmapCleaner loaded ( " << timer.elapsedSeconds() << " second )";
        }
        _renderPassManager->put("ClipmapCleaner", _clipmapCleaner.get());

        _clipmapCopyAlpha = std::make_unique<CopyAlpha>(_device);
        {
            CPUTimer timer;
            _clipmapCopyAlpha->createDescriptors(voxelOpacityView, voxelRadianceView, voxelSampler)
                              .createPipeline();
            VFS_INFO << "CopyAlpha loaded ( " << timer.elapsedSeconds() << " second )";
        }
        _renderPassManager->put("CopyAlpha", _clipmapCopyAlpha.get());
        return true;
    }

    bool Application::buildSVOMethodPasses(void)
    {
        return true;
    }

    void Application::processKeyInput(uint32_t key, bool pressed)
    {
        if (pressed)
        {
            if (key == GLFW_KEY_ESCAPE)
            {
                glfwSetWindowShouldClose(_window->getWindowHandle(), GLFW_TRUE);
            }
        }
    }
}