// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <SceneManager.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/Descriptors/DescriptorSetLayout.h>
#include <VulkanFramework/Descriptors/DescriptorPool.h>
#include <tinyfiledialogs/tinyfiledialogs.h>
#include <GUI/ImGuiUtil.h>
#include <imgui/imgui.h>
#include <GLTFScene.h>

namespace vfs
{
	SceneManager::SceneManager(const QueuePtr& queue, VertexFormat format)
	{
		assert(initialize(queue, format));
	}

	SceneManager::~SceneManager()
	{
		destroySceneManager();
	}

	bool SceneManager::initialize(const QueuePtr& queue, VertexFormat format)
	{
		_device = queue->getDevicePtr();
		_queue = queue;
		_commonFormat = format;

		const std::vector<VkDescriptorPoolSize> poolSizes = {
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER , 2},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kMaxNumTexturePerScene },
		};
		_descPool = std::make_shared<DescriptorPool>(_device, poolSizes, kMaxNumScenes, 0);

		_descLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descLayout->addBinding(VK_SHADER_STAGE_VERTEX_BIT, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0);
		_descLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0);
		_descLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 2, kMaxNumTexturePerScene, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
		return _descLayout->createDescriptorSetLayout(0);
	}

	void SceneManager::destroySceneManager(void)
	{
		_scenes.clear();
		_descLayout.reset();
		_queue.reset();
		_device.reset();
	}

	void SceneManager::addScene(const char* scenePath)
	{
		std::shared_ptr<GLTFScene> scene = std::make_shared<GLTFScene>();
		if (scene->initialize(_device, scenePath, _queue, _commonFormat))
		{
			// Allocate descriptor set
			scene->allocateDescriptor(_descPool, _descLayout);

			// Update total bounding box
			_sceneBoundingBox.updateBoundingBox(scene->getSceneBoundingBox());

			_scenes.emplace_back(std::move(scene));
		}
	}

	void SceneManager::cmdDraw(VkCommandBuffer cmdBuffer, const PipelineLayoutPtr& pipelineLayout,
							   const uint32_t pushConstOffset)
	{
		for (std::shared_ptr<GLTFScene>& scene : _scenes)
		{
			scene->cmdDraw(cmdBuffer, pipelineLayout, pushConstOffset);
		}
	}

	void SceneManager::drawGUI(void)
	{
		constexpr const char* kSceneExtensionFilter[] = { "*.gltf" };
		static char scenePathBuf[256];

		if (ImGui::TreeNode("Scene Settings"))
        {
            if (ImGui::BeginPopupModal("Scene Load", nullptr,
                ImGuiWindowFlags_AlwaysAutoResize | 
				ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove)) 
			{
                ImGui::FileOpen("GLTF Filename", "...", scenePathBuf, 256, "GLTF Filename", 1, kSceneExtensionFilter);

                float buttonWidth = (ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

                if (ImGui::Button("Load", { buttonWidth, 0 })) {
					addScene(scenePathBuf);
					ImGui::CloseCurrentPopup();
				}
				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();
				if (ImGui::Button("Cancel", { buttonWidth, 0 }))
                    ImGui::CloseCurrentPopup();

                ImGui::EndPopup();
            }

            if (ImGui::MenuItem("Load")) {
                ImGui::OpenPopup("Scene Load");
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("Load Scene");
                ImGui::EndTooltip();
            }

			for (std::shared_ptr<GLTFScene>& scene : _scenes)
			{
				scene->drawGUI();
			}
            ImGui::TreePop();
        }

	}

	std::vector<VkVertexInputBindingDescription> SceneManager::getVertexInputBindingDesc(uint32_t bindOffset) const
	{
		constexpr VertexFormat kFormats[] = {
			VertexFormat::Position3, VertexFormat::Normal3, VertexFormat::TexCoord2, VertexFormat::Tangent4
		};
		std::vector<VkVertexInputBindingDescription> bindingDescs;

		uint32_t bindingPoint{ bindOffset };
		for (uint32_t i = 0; i < sizeof(kFormats) / sizeof(VertexFormat); ++i)
		{
			if (static_cast<int>(_commonFormat & kFormats[i]))
			{
				bindingDescs.push_back({
					bindingPoint, VertexHelper::GetNumBytes(kFormats[i]), VK_VERTEX_INPUT_RATE_VERTEX
				});
				bindingPoint += 1;
			}
		}

		return bindingDescs;
	}

	std::vector<VkVertexInputAttributeDescription> SceneManager::getVertexInputAttribDesc(uint32_t bindOffset) const
	{
		std::vector<VkVertexInputAttributeDescription> attribDescs;

		VkVertexInputAttributeDescription attrib = {};

		uint32_t location{ 0 }, bindingPoint{ bindOffset };
		if (static_cast<int>(_commonFormat & VertexFormat::Position3))
		{
			attrib.format = VK_FORMAT_R32G32B32_SFLOAT;
			attrib.binding = bindingPoint;
			attrib.offset = 0;
			attrib.location = location;
			location += 1;
			bindingPoint += 1;
			attribDescs.emplace_back(attrib);
		}
		if (static_cast<int>(_commonFormat & VertexFormat::Normal3))
		{
			attrib.format = VK_FORMAT_R32G32B32_SFLOAT;
			attrib.binding = bindingPoint;
			attrib.offset = 0;
			attrib.location = location;
			location += 1;
			bindingPoint += 1;
			attribDescs.emplace_back(attrib);
		}
		if (static_cast<int>(_commonFormat & VertexFormat::TexCoord2))
		{
			attrib.format = VK_FORMAT_R32G32_SFLOAT;
			attrib.binding = bindingPoint;
			attrib.offset = 0;
			attrib.location = location;
			location += 1;
			bindingPoint += 1;
			attribDescs.emplace_back(attrib);
		}
		if (static_cast<int>(_commonFormat & VertexFormat::Tangent4))
		{
			attrib.format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attrib.binding = bindingPoint;
			attrib.offset = 0;
			attrib.location = location;
			location += 1;
			bindingPoint += 1;
			attribDescs.emplace_back(attrib);
		}

		return attribDescs;
	}

	VkPushConstantRange	SceneManager::getDefaultPushConstant(void) const
	{
		VkPushConstantRange pushConst = {};
		pushConst.offset		= 0;
		pushConst.size			= sizeof(uint32_t) * 2;
		pushConst.stageFlags	= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		return pushConst;
	}
}