// Author : Jihong Shin (snowapril)

#if !defined(VFS_GLTF_SCENE_H)
#define VFS_GLTF_SCENE_H

#include <pch.h>
#include <Util/GLTFLoader.h>
#include <VulkanFramework/DebugUtils.h>
#include <BoundingBox.h>

namespace vfs
{
	class GLTFScene : public GLTFLoader
	{
	public:
		explicit GLTFScene() = default;
		explicit GLTFScene(DevicePtr device, const char* scenePath, 
						   const QueuePtr& queue, VertexFormat format);
				~GLTFScene();

	public:
		bool initialize			(DevicePtr device, const char* scenePath, 
								 const QueuePtr& queue, VertexFormat format);
		void cmdDraw			(VkCommandBuffer cmdBuffer, const PipelineLayoutPtr& pipelineLayout,
								 const uint32_t pushConstOffset);
		void drawGUI			(void);
		void allocateDescriptor	(const DescriptorPoolPtr& pool, const DescriptorSetLayoutPtr& layout);

		inline BoundingBox<glm::vec3> getSceneBoundingBox(void) const
		{
			return BoundingBox<glm::vec3>(_sceneDim.min, _sceneDim.max);
		}
		inline DescriptorSetPtr getDescriptorSet(void) const
		{
			return _descriptorSet;
		}
	private:
		bool uploadBuffer			(void);
		bool uploadImage			(void);
		bool uploadMaterialBuffer	(void);
		bool uploadMatrixBuffer		(void);

	private:
		std::vector<ImagePtr>		_textureImages;
		std::vector<ImageViewPtr>	_textureImageViews;
		std::vector<SamplerPtr>		_textureSamplers;
		std::vector<BufferPtr>		_vertexBuffers;
		BufferPtr					_indexBuffer	 {		nullptr		  };
		DevicePtr					_device			 {		nullptr		  };
		QueuePtr					_queue			 {		nullptr		  };
		VertexFormat				_format			 { VertexFormat::None };
		BufferPtr					_materialBuffer	 {		nullptr		  };
		BufferPtr					_matrixBuffer	 {		nullptr		  };
		DescriptorSetPtr			_descriptorSet	 {		nullptr		  };
		DebugUtils					_debugUtil;
	};
}

#endif