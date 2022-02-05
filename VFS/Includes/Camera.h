// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_CAMERA_H)
#define VULKAN_FRAMEWORK_CAMERA_H

#include <VulkanFramework/pch.h>
struct GLFWwindow;

namespace vfs
{
	class Camera : NonCopyable
	{
	public:
		explicit Camera() = default;
		explicit Camera(WindowPtr window, const DevicePtr& device, const uint32_t frameCount);
				~Camera() = default;

	public:
		bool initialize		(WindowPtr window, const DevicePtr& device, const uint32_t frameCount);
		void processInput	(GLFWwindow* window, const float deltaTime);
		void updateCamera	(const uint32_t currentFrameIndex);

		inline glm::vec3 getOriginPos(void) const
		{
			return _position;
		}
		inline DescriptorSetPtr getDescriptorSet(const uint32_t frameIndex) const
		{
			return _descriptorSets[frameIndex];
		}
		inline DescriptorSetLayoutPtr getDescriptorSetLayout(void) const
		{
			return _descriptorLayout;
		}

	private:
		struct CameraUBO
		{
			glm::mat4	viewProj;
			glm::mat4	viewProjInv;
			glm::vec3	eyePos;
			int			padding;
		};

		std::vector<BufferPtr>			_uniformBuffers;
		std::vector<DescriptorSetPtr>	_descriptorSets;

		glm::mat4	_viewMatrix		= glm::mat4(1.0f);
		glm::mat4	_projMatrix		= glm::mat4(1.0f);
		glm::vec3	_position		= glm::vec3(0.0f);
		glm::vec3	_direction		= glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3	_up				= glm::vec3(0.0f, -1.0f, 0.0f);
		glm::dvec2	_lastCursorPos	= glm::dvec2(0.0, 0.0);

		DescriptorSetLayoutPtr  _descriptorLayout{ nullptr	};
		DescriptorPoolPtr		_descriptorPool	 { nullptr	};
		WindowPtr				_window			 { nullptr	};
		float					_fovy			 {	60.0f	};
		float					_speed			 {  10.0f	};
		bool					_firstCall		 {	true	};
	};
};

#endif