// Author : Jihong Shin (snowapril)

#if !defined(VFS_DIRECTIONAL_LIGHT_H)
#define VFS_DIRECTIONAL_LIGHT_H

#include <Common/pch.h>
#include <Common/Utils.h>

namespace vfs
{
	class DirectionalLight : NonCopyable
	{
	public:
		explicit DirectionalLight(const DevicePtr& device);
				~DirectionalLight() = default;

	public:
		DirectionalLight&	setTransform			(glm::vec3 origin, glm::vec3 direction);
		DirectionalLight&	setColorAndIntensity	(glm::vec3 color, float intensity);
		DirectionalLight&	createShadowMap			(VkExtent2D resolution);
		VkExtent2D			getShadowMapResolution	(void) const;

		inline ImagePtr getShadowMap(void) const
		{
			return _shadowMap;
		}
		inline ImageViewPtr getShadowMapView(void) const
		{
			return _shadowMapView;
		}
		inline BufferPtr getViewProjectionBuffer(void) const
		{
			return _viewProjBuffer;
		}
		inline BufferPtr getLightDescBuffer(void) const
		{
			return _lightDescBuffer;
		}
	private:
		DevicePtr		_device;
		glm::vec3		_direction	{ 0.0f, -1.0f, 0.0f };
		ImagePtr		_shadowMap;
		ImageViewPtr	_shadowMapView;
		SamplerPtr		_shadowMapSampler;
		BufferPtr		_viewProjBuffer;
		BufferPtr		_lightDescBuffer;
	};
};

#endif