// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <Common/Logger.h>
#include <GUI/UIRenderer.h>
#include <GUI/ImGuiUtil.h>
#include <VulkanFramework/Commands/CommandPool.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/Utils.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Sync/Fence.h>
#include <VulkanFramework/Window.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/FrameLayout.h>
#include <VulkanFramework/Images/ImageView.h>
#include <VulkanFramework/Images/Sampler.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

namespace vfs
{
	UIRenderer::UIRenderer(std::shared_ptr<Window> window, std::shared_ptr<Device> device, 
						   QueuePtr graphicsQueue, VkRenderPass renderPass)
	{
		assert(initialize(window, device, graphicsQueue, renderPass));
	}

	UIRenderer::~UIRenderer()
	{
		destroyUIRenderer();
	}

	void UIRenderer::destroyUIRenderer()
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		vkDestroyDescriptorPool(_device->getDeviceHandle(), _descriptorPool, nullptr);
	}

	bool UIRenderer::initialize(std::shared_ptr<Window> window, std::shared_ptr<Device> device, 
								QueuePtr graphicsQueue, VkRenderPass renderPass)
	{
		_device			= device;
		_graphicsQueue	= graphicsQueue;

		if (!initializeDescriptorPool())
		{
			VFS_ERROR << "Failed to initialize descriptor pool for imgui";
			return false;
		}

		if (!initializeImGuiContext(window, renderPass))
		{
			VFS_ERROR << "Failed to initialize imgui Context";
			return false;
		}

		return true;
	}

	bool UIRenderer::initializeDescriptorPool(void)
	{
		// TODO(snowapril) : Although imgui official sample use below pool size, we must reduce it
		VkDescriptorPoolSize poolSize[] = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER,				 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,			 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,			 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,	 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,	 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,		 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,		 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,		 1000 },
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.pNext			= nullptr;
		descriptorPoolInfo.poolSizeCount	= sizeof(poolSize) / sizeof(VkDescriptorPoolSize);
		descriptorPoolInfo.pPoolSizes		= poolSize;
		descriptorPoolInfo.maxSets			= 1000; // TODO(snowapril) : same with above TODO
		descriptorPoolInfo.flags			= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		if (vkCreateDescriptorPool(_device->getDeviceHandle(), &descriptorPoolInfo, nullptr, &_descriptorPool) != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}

	bool UIRenderer::initializeImGuiContext(std::shared_ptr<Window> window, VkRenderPass renderPass)
	{
		ImGui::CreateContext();
		ImGui::StyleCinder(nullptr);
		// ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForVulkan(window->getWindowHandle(), true);
		ImGui_ImplVulkan_InitInfo imGuiInitData = {};
		imGuiInitData.Device			= _device->getDeviceHandle();
		imGuiInitData.DescriptorPool	= _descriptorPool;
		imGuiInitData.PhysicalDevice	= _device->getPhysicalDeviceHandle();
		imGuiInitData.Allocator			= nullptr;
		imGuiInitData.Instance			= _device->getVulkanInstance();
		imGuiInitData.QueueFamily		= _graphicsQueue->getFamilyIndex();
		imGuiInitData.Queue				= _graphicsQueue->getQueueHandle();
		imGuiInitData.PipelineCache		= VK_NULL_HANDLE;
		imGuiInitData.CheckVkResultFn	= nullptr;

		// TODO(snowapril) : minImageCount and imageCount are hard-coded now. need to fix
		imGuiInitData.MinImageCount	= 10; // TODO(snowapril) : this may cause error
		imGuiInitData.ImageCount	= 10; // TODO(snowapril) : this may cause error
		imGuiInitData.MSAASamples	= getMaximumSampleCounts(_device);

		if (!ImGui_ImplVulkan_Init(&imGuiInitData, renderPass))
		{
			return false;
		}
		return true;
	}

	bool UIRenderer::createFontTexture(const CommandPoolPtr& cmdPool)
	{
		CommandBuffer cmdBuffer(cmdPool->allocateCommandBuffer());

		cmdBuffer.beginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer.getHandle());

		cmdBuffer.endRecord();

		Fence fence(_device, 1, 0);
		_graphicsQueue->submitCmdBuffer({ cmdBuffer }, &fence);
		if (!fence.waitForAllFences(UINT64_MAX))
		{
			return false;
		}
		ImGui_ImplVulkan_DestroyFontUploadObjects();

		return true;
	}

	void UIRenderer::beginUIRender(void)
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void UIRenderer::endUIRender(const FrameLayout* frameLayout)
	{
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frameLayout->commandBuffer);
	}
}