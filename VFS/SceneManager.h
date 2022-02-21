// Author : Jihong Shin (snowapril)

#if !defined(VFS_SCENE_MANAGER_H)
#define VFS_SCENE_MANAGER_H

#include <pch.h>
#include <GLTFScene.h>
#include <Common/VertexFormat.h>

namespace vfs
{
	class GLTFScene;

	class SceneManager : public GLTFLoader
	{
	public:
		explicit SceneManager() = default;
		explicit SceneManager(const QueuePtr& queue, VertexFormat format);
				~SceneManager();

		static constexpr uint32_t kMaxNumScenes			 =  10u;
		static constexpr uint32_t kMaxNumTexturePerScene = 100u;
	public:
		bool initialize			(const QueuePtr& queue, VertexFormat format);
		void destroySceneManager(void);
		void addScene			(const char* scenePath);

		void cmdDraw			(VkCommandBuffer cmdBuffer, const PipelineLayoutPtr& pipelineLayout,
								 const uint32_t pushConstOffset);
		void drawGUI			(void);

		std::vector<VkVertexInputBindingDescription>	getVertexInputBindingDesc	(uint32_t bindOffset) const;
		std::vector<VkVertexInputAttributeDescription>	getVertexInputAttribDesc	(uint32_t bindOffset) const;
		VkPushConstantRange								getDefaultPushConstant		(void) const;

		inline const BoundingBox<glm::vec3>& getSceneBoundingBox(void) const
		{
			return _sceneBoundingBox;
		}
		inline std::shared_ptr<GLTFScene> getScenePtr(size_t index)
		{
			assert(index < _scenes.size());
			return _scenes[index];
		}
		inline DescriptorSetLayoutPtr getDescriptorLayout(void) const
		{
			return _descLayout;
		}

	private:
		DevicePtr				_device;
		QueuePtr				_queue;
		DescriptorPoolPtr		_descPool;
		DescriptorSetLayoutPtr	_descLayout;
		std::vector<std::shared_ptr<GLTFScene>> _scenes;
		BoundingBox<glm::vec3>	_sceneBoundingBox;
		VertexFormat _commonFormat;
	};
}

#endif