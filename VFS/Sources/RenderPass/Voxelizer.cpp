// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <RenderPass/Voxelizer.h>
#include <VulkanFramework/CommandBuffer.h>
#include <VulkanFramework/Buffer.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/DescriptorSetLayout.h>
#include <VulkanFramework/DescriptorSet.h>
#include <VulkanFramework/DescriptorPool.h>
#include <VulkanFramework/Image.h>
#include <VulkanFramework/ImageView.h>
#include <VulkanFramework/Sampler.h>
#include <VulkanFramework/Framebuffer.h>
#include <VulkanFramework/RenderPass.h>
#include <VulkanFramework/GraphicsPipeline.h>
#include <VulkanFramework/PipelineLayout.h>
#include <VulkanFramework/PipelineConfig.h>
#include <Camera.h>
#include <VulkanFramework/FrameLayout.h>
#include <VulkanFramework/Utils.h>
#include <RenderPass/RenderPassManager.h>
#include <GLTFScene.h>

namespace vfs
{
	Voxelizer::Voxelizer(DevicePtr device, uint32_t voxelResolution)
		: RenderPassBase(device), 
		  _sampleCount(getMaximumSampleCounts(device)), 
		  _voxelResolution(voxelResolution)
	{
		// Do nothing
	}

	Voxelizer::~Voxelizer()
	{
		// Do nothing
	}

	Voxelizer& Voxelizer::createAttachments(VkExtent2D resolution)
	{
		if (_attachments.empty() == false)
		{
			_attachments.clear();
		}
		
		VkImageCreateInfo imageInfo = Image::GetDefaultImageCreateInfo();
		imageInfo.extent		= { resolution.width, resolution.height, 1 };
		imageInfo.format		= VK_FORMAT_D32_SFLOAT;
		imageInfo.usage			= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.imageType		= VK_IMAGE_TYPE_2D;
		imageInfo.samples		= _sampleCount;
		ImagePtr depthImage		= std::make_shared<Image>(_device->getMemoryAllocator(), VMA_MEMORY_USAGE_GPU_ONLY, imageInfo);
		ImageViewPtr imageView	= std::make_shared<ImageView>(_device, depthImage, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

		_attachments.push_back({ depthImage, imageView });
		return *this;
	}

	Voxelizer& Voxelizer::createRenderPass(void)
	{
		assert(_attachments.empty() == false);

		VkAttachmentDescription depthDesc = {};
		depthDesc.format		 = _attachments[0].image->getImageFormat();
		depthDesc.samples		 = _sampleCount;
		depthDesc.loadOp		 = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthDesc.storeOp		 = VK_ATTACHMENT_STORE_OP_STORE;
		depthDesc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthDesc.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		depthDesc.finalLayout	 = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		_renderPass = std::make_shared<RenderPass>();
		assert(_renderPass->initialize(_device, { depthDesc }, {}, true));
		return *this;
	}

	Voxelizer& Voxelizer::createFramebuffer(VkExtent2D resolution)
	{
		if (_framebuffer != nullptr)
		{
			_framebuffer.reset();
		}

		assert(_attachments.empty() == false && _renderPass != nullptr);
		const std::vector<VkImageView> depthView = { _attachments[0].imageView->getImageViewHandle() };
		_framebuffer = std::make_shared<Framebuffer>(_device, depthView, _renderPass->getHandle(), resolution);
		return *this;
	}

	Voxelizer& Voxelizer::createDescriptors(void)
	{
		// Descriptors for voxelization info buffers
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
		};
		_voxelDescPool = std::make_shared<DescriptorPool>(_device, poolSizes, DEFAULT_CLIP_REGION_COUNT, 0);

		_voxelDescLayout = std::make_shared<DescriptorSetLayout>(_device);
		_voxelDescLayout->addBinding(VK_SHADER_STAGE_GEOMETRY_BIT, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_voxelDescLayout->addBinding(VK_SHADER_STAGE_GEOMETRY_BIT, 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_voxelDescLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_voxelDescLayout->createDescriptorSetLayout(0);

		for (uint32_t i = 0; i < DEFAULT_CLIP_REGION_COUNT; ++i)
		{
			_voxelDescSets[i] = std::make_shared<DescriptorSet>(_device, _voxelDescPool, _voxelDescLayout, 1);
			_clipmapBuffers[i] = std::make_shared<Buffer>(_device->getMemoryAllocator(), sizeof(VoxelizationDesc),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
			_voxelDescSets[i]->updateUniformBuffer({ _clipmapBuffers[i] }, 2, 1);
			_viewportBuffers[i] = std::make_shared<Buffer>(_device->getMemoryAllocator(), sizeof(glm::uvec2) * 3,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
			_voxelDescSets[i]->updateUniformBuffer({ _viewportBuffers[i] }, 0, 1);
			_viewProjBuffers[i] = std::make_shared<Buffer>(_device->getMemoryAllocator(), sizeof(glm::mat4) * 6,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
			_voxelDescSets[i]->updateUniformBuffer({ _viewProjBuffers[i] }, 1, 1);
		}

		return *this;
	}

	Voxelizer& Voxelizer::createVoxelClipmap(uint32_t extentLevel0)
	{
		const VkExtent3D voxelDimension = getClipmapResolution();

		VkImageCreateInfo imageInfo = Image::GetDefaultImageCreateInfo();
		imageInfo.extent		= voxelDimension;
		imageInfo.format		= VK_FORMAT_R8G8B8A8_UNORM;
		imageInfo.usage			= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imageInfo.imageType		= VK_IMAGE_TYPE_3D;
		imageInfo.samples		= VK_SAMPLE_COUNT_1_BIT;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		// Create opacity voxel image and its view
		_voxelOpacity		= std::make_shared<Image>(_device->getMemoryAllocator(), VMA_MEMORY_USAGE_GPU_ONLY, imageInfo);
		_voxelOpacityView	= std::make_shared<ImageView>(_device, _voxelOpacity, VK_IMAGE_ASPECT_COLOR_BIT, 1);

		// Create radiance voxel image and its view
		// imageInfo.format = VK_FORMAT_R32_UINT;
		imageInfo.flags		= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
		_voxelRadiance		= std::make_shared<Image>(_device->getMemoryAllocator(), VMA_MEMORY_USAGE_GPU_ONLY, imageInfo);
		_voxelRadianceView	= std::make_shared<ImageView>(_device, _voxelRadiance, VK_IMAGE_ASPECT_COLOR_BIT, 1);

		VkImageViewCreateInfo radianceR32ViewInfo = ImageView::GetDefaultImageViewInfo();
		radianceR32ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
		radianceR32ViewInfo.format = VK_FORMAT_R32_UINT;
		radianceR32ViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		_voxelRadianceR32View = std::make_shared<ImageView>(_device, _voxelRadiance, radianceR32ViewInfo);

		// Create voxel sampler
		_voxelSampler = std::make_shared<Sampler>(_device, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FILTER_LINEAR, 1.0f);

		// Resource sharing
		_renderPassManager->put("VoxelOpacity",			_voxelOpacity.get());
		_renderPassManager->put("VoxelOpacityView",		_voxelOpacityView.get());
		_renderPassManager->put("VoxelRadiance",		_voxelRadiance.get());
		_renderPassManager->put("VoxelRadianceView",	_voxelRadianceView.get());
		_renderPassManager->put("VoxelRadianceR32View", _voxelRadianceR32View.get());
		_renderPassManager->put("VoxelSampler",			_voxelSampler.get());

		// TODO(snowapril) : select use _DEBUG macro or not (if yes, I should apply it wherever they are being used)
#if defined(_DEBUG)
		_debugUtils.setObjectName(_voxelOpacity->getImageHandle(),				"VoxelOpacity"		  );
		_debugUtils.setObjectName(_voxelOpacityView->getImageViewHandle(),		"VoxelOpacityView"	  );
		_debugUtils.setObjectName(_voxelRadiance->getImageHandle(),				"VoxelRadiance"		  );
		_debugUtils.setObjectName(_voxelRadianceView->getImageViewHandle(),		"VoxelRadianceView"	  );
		_debugUtils.setObjectName(_voxelRadianceR32View->getImageViewHandle(),	"VoxelRadianceR32View");
		_debugUtils.setObjectName(_voxelSampler->getSamplerHandle(),			"VoxelSampler"		  );
#endif
		return *this;
	}

	void Voxelizer::onBeginRenderPass(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
		VkClearValue depthClear;
		depthClear.depthStencil = { 1.0f, 0 };

		cmdBuffer.beginRenderPass(_renderPass, _framebuffer, { depthClear });
	}

	void Voxelizer::onEndRenderPass(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
		cmdBuffer.endRenderPass();
	}

	void Voxelizer::onUpdate(const FrameLayout* frameLayout)
	{
		(void)frameLayout;
	}

	void Voxelizer::processWindowResize(int width, int height)
	{
		createAttachments({ static_cast<uint32_t>(width), static_cast<uint32_t>(height) });
		createFramebuffer({ static_cast<uint32_t>(width), static_cast<uint32_t>(height) });
	}

	void Voxelizer::cmdVoxelize(VkCommandBuffer cmdBufferHandle, const ClipmapRegion& region, uint32_t clipLevel)
	{
		if (_clipmapBuffers[clipLevel] == nullptr)
		{
			_clipmapBuffers[clipLevel] = std::make_shared<Buffer>(_device->getMemoryAllocator(), sizeof(VoxelizationDesc),
																   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
			_voxelDescSets[clipLevel]->updateUniformBuffer({ _clipmapBuffers[clipLevel] }, 2, 1);
		}

		VoxelizationDesc vxDesc;
		vxDesc.regionMinCorner = glm::vec3(region.minCorner) * region.voxelSize;
		vxDesc.regionMaxCorner = glm::vec3(region.minCorner + glm::ivec3(region.extent)) * region.voxelSize;

		ClipmapRegion extendedRegion = region;
		extendedRegion.extent		= extendedRegion.extent + DEFAULT_VOXEL_BORDER;
		extendedRegion.minCorner	-= 1;

		setViewport(cmdBufferHandle, extendedRegion.extent, clipLevel);
		setViewProjection(cmdBufferHandle, extendedRegion, clipLevel);
		
		const std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>* clipmapRegions = _renderPassManager->get<std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>>("ClipmapRegions");
		const ClipmapRegion& targetRegion = clipmapRegions->at(clipLevel);
		vxDesc.clipLevel		= clipLevel;
		vxDesc.clipMaxExtent	= targetRegion.extent.x * targetRegion.voxelSize;
		vxDesc.voxelSize		= targetRegion.voxelSize;

		if (clipLevel != 0)
		{
			const ClipmapRegion& prevRegion = clipmapRegions->at(clipLevel - 1);
			vxDesc.prevRegionMinCorner = glm::vec3(prevRegion.minCorner) * prevRegion.voxelSize;
			vxDesc.prevRegionMaxCorner = glm::vec3(prevRegion.minCorner + glm::ivec3(prevRegion.extent)) * prevRegion.voxelSize;
			// TODO(snowapril) : downsample transition region size
		}
		
		vxDesc.clipmapResolution = static_cast<int32_t>(_voxelResolution);

		// Upload Voxelization description staging buffer
		CommandBuffer cmdBuffer(cmdBufferHandle);
		_clipmapBuffers[clipLevel]->uploadData(&vxDesc, sizeof(VoxelizationDesc));
	}

	void Voxelizer::setViewport(VkCommandBuffer cmdBufferHandle, glm::uvec3 viewportSize, uint32_t clipLevel)
	{
		assert(clipLevel < DEFAULT_CLIP_REGION_COUNT);
		if (_viewportBuffers[clipLevel] == nullptr)
		{
			_viewportBuffers[clipLevel] = std::make_shared<Buffer>(_device->getMemoryAllocator(), sizeof(glm::uvec2) * 3,
																   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
			_voxelDescSets[clipLevel]->updateUniformBuffer({ _viewportBuffers[clipLevel] }, 0, 1);
		}

		CommandBuffer cmdBuffer(cmdBufferHandle);
		std::vector<VkViewport> viewports{
			{ 0.0f, 0.0f, static_cast<float>(viewportSize.z), static_cast<float>(viewportSize.y), 0.0f, 1.0f},
			{ 0.0f, 0.0f, static_cast<float>(viewportSize.x), static_cast<float>(viewportSize.z), 0.0f, 1.0f},
			{ 0.0f, 0.0f, static_cast<float>(viewportSize.x), static_cast<float>(viewportSize.y), 0.0f, 1.0f},
		};
		cmdBuffer.setViewport(viewports);

		std::vector<VkRect2D> scissors {
			{ {0, 0 }, { viewportSize.z, viewportSize.y } },
			{ {0, 0 }, { viewportSize.x, viewportSize.z } },
			{ {0, 0 }, { viewportSize.x, viewportSize.y } }
		};
		cmdBuffer.setScissor(scissors);
		
		glm::uvec2 viewportSizes[] = {
			{viewportSize.z, viewportSize.y},
			{viewportSize.x, viewportSize.z},
			{viewportSize.x, viewportSize.y}
		};

		_viewportBuffers[clipLevel]->uploadData(&viewportSizes[0], sizeof(glm::uvec2) * 3);
	}

	void Voxelizer::setViewProjection(VkCommandBuffer cmdBufferHandle, const ClipmapRegion& region, uint32_t clipLevel)
	{
		assert(clipLevel < DEFAULT_CLIP_REGION_COUNT);
		if (_viewProjBuffers[clipLevel] == nullptr)
		{
			_viewProjBuffers[clipLevel] = std::make_shared<Buffer>(_device->getMemoryAllocator(), sizeof(glm::mat4) * 6,
																   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
			_voxelDescSets[clipLevel]->updateUniformBuffer({ _viewProjBuffers[clipLevel] }, 1, 1);
		}

		const glm::vec3 regionGlobal	= glm::vec3(region.extent) * region.voxelSize;
		const glm::vec3 minCornerGlobal = glm::vec3(region.minCorner) * region.voxelSize;
		const glm::vec3 eye				= minCornerGlobal + glm::vec3(0.0f, 0.0f, regionGlobal.z);

		glm::mat4 viewProj[6];
		viewProj[0] = glm::ortho(-regionGlobal.z, regionGlobal.z, -regionGlobal.y, regionGlobal.y, -regionGlobal.x, regionGlobal.x) *
					  glm::lookAt(eye, eye + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		viewProj[1] = glm::ortho(-regionGlobal.x, regionGlobal.x, -regionGlobal.z, regionGlobal.z, -regionGlobal.y, regionGlobal.y) *
				      glm::lookAt(eye, eye + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
		viewProj[2] = glm::ortho(-regionGlobal.x, regionGlobal.x, -regionGlobal.y, regionGlobal.y, -regionGlobal.z, regionGlobal.z) *
					  glm::lookAt(minCornerGlobal, minCornerGlobal + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		
		for (uint32_t i = 0; i < 3; ++i)
		{
			viewProj[i + 3] = glm::inverse(viewProj[i]);
		}
		
		CommandBuffer cmdBuffer(cmdBufferHandle);
		_viewProjBuffers[clipLevel]->uploadData(&viewProj[0], sizeof(glm::mat4) * 6);
	}
};