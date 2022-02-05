// Author : Jihong Shin (snowapril)

#if !defined(RENDER_ENGINE_PCH_H)
#define RENDER_ENGINE_PCH_H

#include <VulkanFramework/NonCopyable.h>
#include <vector>
#include <memory>

#define NOMINMAX
#include <windows.h>

#pragma warning (push)
#pragma warning (disable :  4191)
#pragma warning (disable :  4296)
#pragma warning (disable :  4324)
#pragma warning (disable :  4355)
#pragma warning (disable :  4464)
#pragma warning (disable :  4701)
#pragma warning (disable :  4819)
#pragma warning (disable :  4946)
#pragma warning (disable :  5026)
#pragma warning (disable :  5027)
#pragma warning (disable :  5038)
#pragma warning (disable :  6262)
#pragma warning (disable :  6386)
#pragma warning (disable :  6387)
#pragma warning (disable : 26110)
#pragma warning (disable : 26495)
#pragma warning (disable : 26812)

#define GLM_FORCE_SIZE_T_LENGTH
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <tinygltf/tiny_gltf.h>
#include <vma/vk_mem_alloc.h>

#pragma warning (pop)

// forward declarations
namespace vfs
{
	class Buffer;
	class BufferView;
	class Camera;
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
	class Renderer;
	class RenderPass;
	class Sampler;
	class Scene;
	class SwapChain;
	class Image;
	class ImageView;
	class Semaphore;
	class Window;

	using BufferPtr				 = std::shared_ptr<Buffer>;
	using BufferViewPtr			 = std::shared_ptr<BufferView>;
	using CameraPtr				 = std::shared_ptr<Camera>;
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
	using RendererPtr			 = std::shared_ptr<Renderer>;
	using RenderPassPtr			 = std::shared_ptr<RenderPass>;
	using SamplerPtr			 = std::shared_ptr<Sampler>;
	using ScenePtr				 = std::shared_ptr<Scene>;
	using SwapChainPtr			 = std::shared_ptr<SwapChain>;
	using ImagePtr				 = std::shared_ptr<Image>;
	using ImageViewPtr			 = std::shared_ptr<ImageView>;
	using WindowPtr				 = std::shared_ptr<Window>;
	using SemaphorePtr			 = std::shared_ptr<Semaphore>;
};

#endif