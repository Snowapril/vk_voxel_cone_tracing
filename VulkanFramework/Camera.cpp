// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <GLFW/glfw3.h>
#include <VulkanFramework/Camera.h>
#include <VulkanFramework/Window.h>
#include <VulkanFramework/Buffer.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/DescriptorPool.h>
#include <VulkanFramework/DescriptorSet.h>
#include <VulkanFramework/DescriptorSetLayout.h>
#include <iostream>
#include <Common/Utils.h>

namespace vfs
{
	Camera::Camera(WindowPtr window, const DevicePtr& device, const uint32_t frameCount)
	{
		assert(initialize(window, device, frameCount));
	}

	bool Camera::initialize(WindowPtr window, const DevicePtr& device, const uint32_t frameCount)
	{
		_window = window;

		// Initialize uniform buffer for camera view & viewproj
		_uniformBuffers.reserve(frameCount);
		for (uint32_t i = 0; i < frameCount; ++i)
		{
			_uniformBuffers.emplace_back(std::make_shared<Buffer>(device->getMemoryAllocator(), sizeof(CameraUBO),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU));
		}

		// Initialize descriptor pool
		const std::vector<VkDescriptorPoolSize> poolSizes = {
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frameCount},
		};
		_descriptorPool = std::make_shared<DescriptorPool>(device, poolSizes, frameCount, 0);

		// Initialize descriptor layout
		_descriptorLayout = std::make_shared<DescriptorSetLayout>(device);
		_descriptorLayout->addBinding(
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0
		);
		if (!_descriptorLayout->createDescriptorSetLayout(0))
		{
			return false;
		}

		// Initialize descriptor set
		_descriptorSets.reserve(frameCount);
		for (uint32_t i = 0; i < frameCount; ++i)
		{
			_descriptorSets.emplace_back(std::make_shared<DescriptorSet>(device, _descriptorPool, _descriptorLayout, 1));
		}

		return true;
	}

	void Camera::processInput(GLFWwindow* window, const float deltaTime)
	{
		double xpos{ 0.0 }, ypos{ 0.0 };
		glfwGetCursorPos(window, &xpos, &ypos);

		const glm::dvec2 cursorPos(xpos, ypos);
		if (_firstCall)
		{
			_lastCursorPos = cursorPos;
			_firstCall = false;
		}

		constexpr float sensitivity = 8e-2f;
		const float xoffset = static_cast<float>(_lastCursorPos.x - cursorPos.x) * sensitivity;
		const float yoffset = static_cast<float>(cursorPos.y - _lastCursorPos.y) * sensitivity;
		_lastCursorPos = cursorPos;

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{
			// create quaternion matrix with up vector and yaw angle.
			auto yawQuat = glm::angleAxis(glm::radians(xoffset), _up);
			// create quaternion matrix with right vector and pitch angle.
			auto pitchQuat = glm::angleAxis(glm::radians(yoffset), glm::cross(_direction, _up));

			_direction = glm::normalize(yawQuat * pitchQuat) * _direction;
		}

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			_position += _direction * _speed * deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		{
			_position -= glm::cross(_direction, _up) * _speed * deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			_position -= _direction * _speed * deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			_position += glm::cross(_direction, _up) * _speed * deltaTime;
		}
	}

	void Camera::updateCamera(const uint32_t currentFrameIndex)
	{
		_viewMatrix		= glm::lookAt(_position, _position + _direction, _up);
		_projMatrix		= glm::perspective(glm::radians(_fovy), _window->getAspectRatio(), 0.01f, 5000.0f);

		CameraUBO ubo = {
			_projMatrix * _viewMatrix, glm::inverse(_viewMatrix) * glm::inverse(_projMatrix), _position
		};

		_uniformBuffers[currentFrameIndex]->uploadData(&ubo, sizeof(ubo));
		_descriptorSets[currentFrameIndex]->updateUniformBuffer({ _uniformBuffers[currentFrameIndex] }, 0, 1);
	}
};