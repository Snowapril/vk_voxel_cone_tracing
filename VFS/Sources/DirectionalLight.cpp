// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <DirectionalLight.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Image.h>
#include <VulkanFramework/ImageView.h>
#include <VulkanFramework/Sampler.h>
#include <VulkanFramework/Buffer.h>
#include <Shaders/light.glsl>

namespace vfs
{
	DirectionalLight::DirectionalLight(const DevicePtr& device)
		: _device(device)
	{
		// Do nothing
	}

	DirectionalLight& DirectionalLight::setTransform(glm::vec3 origin, glm::vec3 direction)
	{
		if (_viewProjBuffer == nullptr)
		{
			_viewProjBuffer = std::make_shared<Buffer>(_device->getMemoryAllocator(), sizeof(glm::mat4),
													   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
													   VMA_MEMORY_USAGE_CPU_TO_GPU);
		}
		
		_direction = glm::normalize(direction);

		// TODO(snowapril) : add control to zFar and zNear of light projection matrix
		constexpr glm::vec3 kOrthoHalfResolution{ 16.0f, 16.0f, 16.0f };
		
		const glm::mat4 ortho = glm::ortho(
			-kOrthoHalfResolution.x, kOrthoHalfResolution.x,
			-kOrthoHalfResolution.y, kOrthoHalfResolution.y,
			-kOrthoHalfResolution.z, kOrthoHalfResolution.z
		);
		const glm::mat4 view = glm::lookAt(origin, origin + _direction, glm::vec3(0.0f, 1.0f, 0.0f));

		// const glm::mat4 viewProj = ortho * view;
		glm::mat4 uploadData[] = { view, ortho };
		_viewProjBuffer->uploadData(uploadData, sizeof(uploadData));
		return *this;
	}

	DirectionalLight& DirectionalLight::createShadowMap(VkExtent2D shadowMapResolution)
	{
		const VkExtent3D attachmentExtent = { shadowMapResolution.width, shadowMapResolution.height, 1 };
		
		VkImageCreateInfo imageInfo = Image::GetDefaultImageCreateInfo();
		imageInfo.extent	= attachmentExtent;
		imageInfo.format	= VK_FORMAT_D32_SFLOAT;
		imageInfo.usage		= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.samples	= VK_SAMPLE_COUNT_1_BIT;
		
		_shadowMap			= std::make_shared<Image>(_device->getMemoryAllocator(), VMA_MEMORY_USAGE_GPU_ONLY, imageInfo);
		_shadowMapView		= std::make_shared<ImageView>(_device, _shadowMap, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		_shadowMapSampler	= std::make_shared<Sampler>(_device, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FILTER_LINEAR, 0.0f);

		return *this;
	}

	DirectionalLight& DirectionalLight::setColorAndIntensity(glm::vec3 color, float intensity)
	{
		if (_lightDescBuffer == nullptr)
		{
			_lightDescBuffer = std::make_shared<Buffer>(_device->getMemoryAllocator(), sizeof(glm::mat4),
														VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
														VMA_MEMORY_USAGE_CPU_TO_GPU);
		}
		DirectionalLightDesc lightDesc;
		lightDesc.direction = _direction;
		lightDesc.color		= color;
		lightDesc.intensity = intensity;

		_lightDescBuffer->uploadData(&lightDesc, sizeof(lightDesc));
		return *this;
	}

	VkExtent2D DirectionalLight::getShadowMapResolution(void) const
	{
		const VkExtent3D shadowMapDimension = _shadowMap->getDimension();
		return { shadowMapDimension.width, shadowMapDimension.height };
	}
};