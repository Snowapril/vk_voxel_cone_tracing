// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_DEBUG_UTILS_H)
#define VULKAN_FRAMEWORK_DEBUG_UTILS_H

#include <VulkanFramework/pch.h>

namespace vfs
{
	class DebugUtils
	{
	public:
		DebugUtils() = default;
		DebugUtils(DevicePtr device) : _device(device) {};

	public:
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
															VkDebugUtilsMessageTypeFlagsEXT flags,
															const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
															void* userData);
		static void GlfwDebugCallback(int errorCode, const char* description);

	public: 
		struct ScopedCmdLabel
		{
			ScopedCmdLabel(VkCommandBuffer cmdBuffer, const char* label)
				: _cmdBuffer(cmdBuffer)
			{
				VkDebugUtilsLabelEXT labelInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, label, {1.0f, 1.0f, 1.0f, 1.0f} };
				vkCmdBeginDebugUtilsLabelEXT(_cmdBuffer, &labelInfo);
			}
			~ScopedCmdLabel()
			{
				vkCmdEndDebugUtilsLabelEXT(_cmdBuffer);
			}
			void insertLabel(const char* label)
			{
				VkDebugUtilsLabelEXT labelInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, label, {1.0f, 1.0f, 1.0f, 1.0f} };
				vkCmdInsertDebugUtilsLabelEXT(_cmdBuffer, &labelInfo);
			}
		private:
			VkCommandBuffer _cmdBuffer { VK_NULL_HANDLE };
		};

		inline ScopedCmdLabel scopeLabel(VkCommandBuffer cmdBuffer, const char* label)
		{
			return ScopedCmdLabel(cmdBuffer, label);
		}
		inline void setObjectName(VkBuffer buffer, const char* name)
		{
			setObjectName(reinterpret_cast<uint64_t>(buffer), name, VK_OBJECT_TYPE_BUFFER);
		}
		inline void setObjectName(VkImage image, const char* name)
		{
			setObjectName(reinterpret_cast<uint64_t>(image), name, VK_OBJECT_TYPE_IMAGE);
		}
		inline void setObjectName(VkImageView imageView, const char* name)
		{
			setObjectName(reinterpret_cast<uint64_t>(imageView), name, VK_OBJECT_TYPE_IMAGE_VIEW);
		}
		inline void setObjectName(VkSampler sampler, const char* name)
		{
			setObjectName(reinterpret_cast<uint64_t>(sampler), name, VK_OBJECT_TYPE_SAMPLER);
		}
		inline void setObjectName(VkPipeline pipeline, const char* name)
		{
			setObjectName(reinterpret_cast<uint64_t>(pipeline), name, VK_OBJECT_TYPE_PIPELINE);
		}
		inline void setObjectName(VkRenderPass renderPass, const char* name)
		{
			setObjectName(reinterpret_cast<uint64_t>(renderPass), name, VK_OBJECT_TYPE_RENDER_PASS);
		}
		inline void setObjectName(VkFence fence, const char* name)
		{
			setObjectName(reinterpret_cast<uint64_t>(fence), name, VK_OBJECT_TYPE_FENCE);
		}
		inline void setObjectName(VkSemaphore semaphore, const char* name)
		{
			setObjectName(reinterpret_cast<uint64_t>(semaphore), name, VK_OBJECT_TYPE_SEMAPHORE);
		}
		inline void setObjectName(VkDescriptorSet descSet, const char* name)
		{
			setObjectName(reinterpret_cast<uint64_t>(descSet), name, VK_OBJECT_TYPE_DESCRIPTOR_SET);
		}
		inline void setObjectName(VkBufferView bufferView, const char* name)
		{
			setObjectName(reinterpret_cast<uint64_t>(bufferView), name, VK_OBJECT_TYPE_BUFFER_VIEW);
		}
	private:
		void setObjectName(uint64_t object, const char* name, VkObjectType type);

	private:
		DevicePtr _device{ nullptr };

	};
}

#endif