#if !defined(VULKAN_FRAMEWORK_FORWARD_DECLARATIONS_H)
#define VULKAN_FRAMEWORK_FORWARD_DECLARATIONS_H

#include <memory>

// forward declarations
namespace vfs
{
	class Buffer;
	class BufferView;
	class CommandPool;
	class DescriptorPool;
	class DescriptorSet;
	class DescriptorSetLayout;
	class Device;
	class Fence;
	class Framebuffer;
	class QueryPool;
	class PipelineBase;
	class PipelineLayout;
	class GraphicsPipeline;
	class ComputePipeline;
	class Queue;
	class RenderPass;
	class Sampler;
	class Image;
	class ImageView;
	class Semaphore;
	class Window;

	using BufferPtr				 = std::shared_ptr<Buffer>;
	using BufferViewPtr			 = std::shared_ptr<BufferView>;
	using CommandPoolPtr		 = std::shared_ptr<CommandPool>;
	using DescriptorPoolPtr		 = std::shared_ptr<DescriptorPool>;
	using DescriptorSetPtr		 = std::shared_ptr<DescriptorSet>;
	using DescriptorSetLayoutPtr = std::shared_ptr<DescriptorSetLayout>;
	using DevicePtr				 = std::shared_ptr<Device>;
	using FencePtr				 = std::shared_ptr<Fence>;
	using FramebufferPtr		 = std::shared_ptr<Framebuffer>;
	using QueryPoolPtr			 = std::shared_ptr<QueryPool>;
	using PipelineBasePtr		 = std::shared_ptr<PipelineBase>;
	using PipelineLayoutPtr		 = std::shared_ptr<PipelineLayout>;
	using GraphicsPipelinePtr	 = std::shared_ptr<GraphicsPipeline>;
	using ComputePipelinePtr	 = std::shared_ptr<ComputePipeline>;
	using QueuePtr				 = std::shared_ptr<Queue>;
	using RenderPassPtr			 = std::shared_ptr<RenderPass>;
	using SamplerPtr			 = std::shared_ptr<Sampler>;
	using ImagePtr				 = std::shared_ptr<Image>;
	using ImageViewPtr			 = std::shared_ptr<ImageView>;
	using WindowPtr				 = std::shared_ptr<Window>;
	using SemaphorePtr			 = std::shared_ptr<Semaphore>;
};

#endif