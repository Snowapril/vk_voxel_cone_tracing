// Author : Jihong Shin (snowapril)

#if !defined(VOXEL_ENGINE_SPARSE_VOXELIZER_H)
#define VOXEL_ENGINE_SPARSE_VOXELIZER_H

#include <memory>
#include <VulkanFramework/RenderPass/Framebuffer.h>
#include <VulkanFramework/Descriptors/DescriptorSet.h>
#include <VulkanFramework/Pipelines/GraphicsPipeline.h>
#include <VulkanFramework/Commands/CommandBuffer.h>

namespace vfs
{
	class CommandPool;
	class Device;
	class SceneManager;
	class Counter;
	class DescriptorPool;
	class DescriptorSetLayout;
	class DirectionalLight;

	class SparseVoxelizer : NonCopyable
	{
	public:
		explicit SparseVoxelizer() = default;
		explicit SparseVoxelizer(std::shared_ptr<Device> device, std::shared_ptr<SceneManager> sceneManager, 
								 DirectionalLight* light, const uint32_t octreeLevel);
				~SparseVoxelizer();

	public:
		void destroyVoxelizer	(void);
		bool initialize			(std::shared_ptr<Device> device, std::shared_ptr<SceneManager> sceneManager,
								 DirectionalLight* light, const uint32_t octreeLevel);
		void preVoxelize		(const CommandPoolPtr& cmdPool);
		void cmdVoxelize		(VkCommandBuffer cmdBuffer);
		void readVoxelFragments	(const CommandPoolPtr& cmdPool, OUT std::vector<glm::uvec3>* readData);

		inline std::shared_ptr<Buffer> getFramgnetList(void) const
		{
			return _fragmentList;
		}
		inline uint32_t getLevel(void) const
		{
			return _level;
		}
		inline uint32_t getVoxelResolution(void) const
		{
			return _voxelResolution;
		}
		inline uint32_t getFragmentCount(void) const
		{
			return _voxelFragmentCount;
		}
	private:
		bool initializeRenderPass		(void);
		bool initializeDescriptors		(DirectionalLight* light);
		bool initializePipelineLayout	(void);
		bool initializePipeline			(void);
		bool initializeFramebuffer		(void);
		
		void setViewport				(CommandBuffer cmdBuffer);
		void setViewProj				(CommandBuffer cmdBuffer);

	private:
		std::unique_ptr<Counter>		_counter				{ nullptr };
		std::shared_ptr<Buffer>			_fragmentList			{ nullptr };
		FramebufferPtr					_framebuffer			{ nullptr };
		GraphicsPipelinePtr				_pipeline				{ nullptr };
		DevicePtr						_device					{ nullptr };
		std::shared_ptr<SceneManager>	_sceneManager			{ nullptr };
		PipelineLayoutPtr				_pipelineLayout			{ nullptr };
		DescriptorPoolPtr				_descriptorPool			{ nullptr };
		DescriptorSetLayoutPtr			_descriptorLayout		{ nullptr };
		DescriptorSetPtr				_descriptorSet			{ nullptr };
		DescriptorPoolPtr				_lightDescriptorPool	{ nullptr };
		DescriptorSetLayoutPtr			_lightDescriptorLayout	{ nullptr };
		DescriptorSetPtr				_lightDescriptorSet		{ nullptr };
		DescriptorPoolPtr				_voxelDescPool			{ nullptr };
		DescriptorSetLayoutPtr			_voxelDescLayout		{ nullptr };
		DescriptorSetPtr				_voxelDescSet			{ nullptr };
		BufferPtr						_viewProjBuffer			{ nullptr };
		BufferPtr						_sceneDimBuffer			{ nullptr };
		RenderPassPtr					_renderPass				{ nullptr };
		SamplerPtr						_shadowSampler			{ nullptr };
		VkSampleCountFlagBits			_sampleCount			{ VK_SAMPLE_COUNT_1_BIT };
		uint32_t						_voxelResolution		{ 0 };
		uint32_t						_voxelFragmentCount		{ 0 };
		uint32_t						_level					{ 0 };
	};
}

#endif