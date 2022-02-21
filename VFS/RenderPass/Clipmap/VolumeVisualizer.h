// Author : Jihong Shin (snowapril)

#if !defined(VFS_VOLUME_VISUALIZER_H)
#define VFS_VOLUME_VISUALIZER_H

#include <pch.h>
#include <VulkanFramework/Pipelines/ComputePipeline.h>
#include <VulkanFramework/Pipelines/GraphicsPipeline.h>

namespace vfs
{
	struct FrameLayout;
	class VolumeVisualizer : NonCopyable
	{
	public:
		explicit VolumeVisualizer() = default;
		explicit VolumeVisualizer(DevicePtr device, CameraPtr camera);
				~VolumeVisualizer() = default;

	public:
		VolumeVisualizer& buildPointClouds		(const CommandPoolPtr& cmdPool, uint32_t resolution);
		VolumeVisualizer& createDescriptorSet	(ImageViewPtr tagetImageView, SamplerPtr targetSampler);
		VolumeVisualizer& createPipelines		(VkRenderPass renderPass);

		void cmdDraw(const FrameLayout* frame);

	private:
		DevicePtr				_device{ nullptr };
		BufferPtr				_volumePointCloudBuffer{ nullptr };
		CameraPtr				_camera{ nullptr };
		GraphicsPipelinePtr		_avcPipeline{ nullptr };
		PipelineLayoutPtr		_pipelineLayout{ VK_NULL_HANDLE };
		DescriptorSetPtr		_descriptorSet;
		DescriptorSetLayoutPtr	_descriptorLayout;
		DescriptorPoolPtr		_descriptorPool;
		std::vector<glm::vec3>	_pointClouds;
		uint32_t				_numPoints{ 0 };
	};
}

#endif