// Author : Jihong Shin (snowapril)

#if !defined(VFS_GLTF_SCENE_H)
#define VFS_GLTF_SCENE_H

#include <Common/pch.h>
#include <Common/GLTFLoader.h>
#include <VulkanFramework/DebugUtils.h>

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
		void cmdDrawGeometryOnly(VkCommandBuffer cmdBuffer, const PipelineLayoutPtr& pipelineLayout);
		void allocateDescriptor	(const DescriptorPoolPtr& pool, const DescriptorSetLayoutPtr& layout);

		std::vector<VkVertexInputBindingDescription>	getVertexInputBindingDesc	(uint32_t bindingPoint) const;
		std::vector<VkVertexInputAttributeDescription>	getVertexInputAttribDesc	(uint32_t bindingPoint) const;
		VkPushConstantRange								getDefaultPushConstant		(void) const;

		inline DescriptorSetLayoutPtr getDescriptorLayout(void) const
		{
			return _descriptorLayout;
		}
		inline DescriptorSetPtr getDescriptorSet(void) const
		{
			return _descriptorSet;
		}
	private:
		bool uploadBuffer			(const QueuePtr& loaderQueue);
		bool uploadImage			(const QueuePtr& loaderQueue);
		bool uploadMaterialBuffer	(const QueuePtr& loaderQueue);
		bool uploadMatrixBuffer		(const QueuePtr& loaderQueue);
		bool initializeDescriptors(void);

	private:
		std::vector<ImagePtr>		_textureImages;
		std::vector<ImageViewPtr>	_textureImageViews;
		std::vector<SamplerPtr>		_textureSamplers;
		std::vector<BufferPtr>		_vertexBuffers;
		BufferPtr					_indexBuffer	 {		nullptr		  };
		DevicePtr					_device			 {		nullptr		  };
		VertexFormat				_format			 { VertexFormat::None };
		BufferPtr					_materialBuffer	 {		nullptr		  };
		BufferPtr					_matrixBuffer	 {		nullptr		  };
		DescriptorSetPtr			_descriptorSet	 {		nullptr		  };
		DescriptorSetLayoutPtr		_descriptorLayout{		nullptr		  };
		DescriptorPoolPtr			_descriptorPool	 {		nullptr		  };
		DebugUtils					_debugUtil;
	};
}

#endif