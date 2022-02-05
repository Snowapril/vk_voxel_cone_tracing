// Author : Jihong Shin (snowapril)

#include <VulkanFramework/pch.h>
#include <VulkanFramework/Images/Sampler.h>
#include <VulkanFramework/Images/Image.h>
#include <VulkanFramework/Device.h>

namespace vfs
{
	Sampler::Sampler(DevicePtr device, VkSamplerAddressMode sampleMode, VkFilter filter, float mipmapLevel)
	{
		assert(initialize(device, sampleMode, filter, mipmapLevel));
	}

	Sampler::~Sampler()
	{
		destroySamplerHandler();
	}

	void Sampler::destroySamplerHandler(void)
	{
		if (_samplerHandle != VK_NULL_HANDLE)
		{
			vkDestroySampler(_device->getDeviceHandle(), _samplerHandle, nullptr);
			_samplerHandle = VK_NULL_HANDLE;
		}
	}

	bool Sampler::initialize(DevicePtr device, VkSamplerAddressMode sampleMode, VkFilter filter, float maxLod)
	{
		_device = device;

		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType			 = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.pNext			 = nullptr;
		samplerInfo.addressModeU	 = sampleMode;
		samplerInfo.addressModeV	 = sampleMode;
		samplerInfo.addressModeW	 = sampleMode;
		samplerInfo.borderColor		 = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.compareOp		 = VK_COMPARE_OP_NEVER;
		samplerInfo.compareEnable	 = VK_FALSE;
		samplerInfo.minFilter		 = filter;
		samplerInfo.magFilter		 = filter;
		samplerInfo.minLod			 = 0.0f;
		samplerInfo.maxLod			 = maxLod;
		samplerInfo.mipmapMode		 = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias		 = 0.0;
		samplerInfo.maxAnisotropy	 = 1.0;
		
		return vkCreateSampler(_device->getDeviceHandle(), &samplerInfo, nullptr, &_samplerHandle)
			== VK_SUCCESS;
	}
}