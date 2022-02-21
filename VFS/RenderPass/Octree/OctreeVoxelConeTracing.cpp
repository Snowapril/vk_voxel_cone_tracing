// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <Util/EngineConfig.h>
#include <RenderPass/Octree/OctreeVoxelConeTracing.h>
#include <RenderPass/RenderPassManager.h>
#include <RenderPass/Clipmap/ClipmapRegion.h>
#include <VulkanFramework/Buffers/Buffer.h>
#include <VulkanFramework/Commands/CommandBuffer.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Images/Image.h>
#include <VulkanFramework/Images/ImageView.h>
#include <VulkanFramework/Images/Sampler.h>
#include <VulkanFramework/Descriptors/DescriptorSet.h>
#include <VulkanFramework/Descriptors/DescriptorSetLayout.h>
#include <VulkanFramework/Descriptors/DescriptorPool.h>
#include <VulkanFramework/Pipelines/GraphicsPipeline.h>
#include <VulkanFramework/Pipelines/PipelineLayout.h>
#include <VulkanFramework/Pipelines/PipelineConfig.h>
#include <VulkanFramework/RenderPass/RenderPass.h>
#include <VulkanFramework/FrameLayout.h>
#include <VulkanFramework/Utils.h>
#include <VulkanFramework/RenderPass/Framebuffer.h>
#include <DirectionalLight.h>
#include <imgui/imgui.h>

namespace vfs
{
	OctreeVoxelConeTracing::OctreeVoxelConeTracing(CommandPoolPtr cmdPool,
												   VkExtent2D resolution)
	: RenderPassBase(cmdPool), _resolution(resolution)
	{
		// Do nothing
	}

	OctreeVoxelConeTracing::~OctreeVoxelConeTracing()
	{
		// Do nothing
	}

	void OctreeVoxelConeTracing::onBeginRenderPass(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
		std::vector<VkClearValue> clearValues(_attachments.size());
		for (size_t i = 0; i < clearValues.size(); ++i)
		{
			clearValues[i].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		}

		cmdBuffer.beginRenderPass(_renderPass, _framebuffer, clearValues);

		VkViewport viewport = {};
		viewport.x			= 0;
		viewport.y			= 0;
		viewport.minDepth	= 0.0f;
		viewport.maxDepth	= 1.0f;
		viewport.width		= static_cast<float>(_resolution.width);
		viewport.height		= static_cast<float>(_resolution.height);
		cmdBuffer.setViewport({ viewport });

		VkRect2D scissor = {};
		scissor.offset			= { 0, 0 };
		scissor.extent.width	= _resolution.width;
		scissor.extent.height	= _resolution.height;
		cmdBuffer.setScissor({ scissor });
	}

	void OctreeVoxelConeTracing::onEndRenderPass(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
		cmdBuffer.endRenderPass();
	}

	void OctreeVoxelConeTracing::onUpdate(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);

		cmdBuffer.bindPipeline(_pipeline);
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 0, { frameLayout->globalDescSet }, {});
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 1, {		_gbufferDescriptorSet }, {});
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 2, {			   _descriptorSet }, {});
		cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 3, {		   _lightDescriptorSet}, {});

		std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>* clipmapRegions =
			_renderPassManager->get<std::array<ClipmapRegion, DEFAULT_CLIP_REGION_COUNT>>("ClipmapRegions");
		assert(clipmapRegions != nullptr);
		const ClipmapRegion& clipmapLevel0 = clipmapRegions->at(0);
		VoxelConeTracingDesc vctDesc;
		vctDesc.renderingMode				= static_cast<uint32_t>(_renderingMode);
		vctDesc.volumeCenter				= (glm::vec3(clipmapLevel0.minCorner) * clipmapLevel0.voxelSize) +
											  (glm::vec3(clipmapLevel0.extent) * clipmapLevel0.voxelSize) * 0.5f;
		vctDesc.voxelSize					= clipmapLevel0.voxelSize;
		vctDesc.volumeDimension				= 256; // TODO(snowapril) : need to replace _voxelREsolution
		vctDesc.traceStartOffset			= _traceStartOffset;
		vctDesc.indirectDiffuseIntensity	= _indirectDiffuseIntensity;
		vctDesc.ambientOcclusionFactor		= _ambientOcclusionFactor;
		vctDesc.minTraceStepFactor			= _minTraceStepFactor;
		vctDesc.indirectSpecularIntensity	= _indirectSpecularIntensity;
		vctDesc.occlusionDecay				= _occlusionDecay;
		vctDesc.enable32Cones				= static_cast<int32_t>(_enable32Cones);

		cmdBuffer.pushConstants(_pipelineLayout->getLayoutHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(VoxelConeTracingDesc), &vctDesc);
		cmdBuffer.draw(4, 1, 0, 0);
	}

	void OctreeVoxelConeTracing::drawGUI(void)
	{
		constexpr const char* kRenderingModeLabels[] = {
			"Diffuse-Only", "Specular-Only", "Normal-Only", "VCT-Start-Level-Only",
			"DirectContribution", "IndirectDiffuse", "IndirectSpecular", "AmbientOcclsion", "CombinedGI"
		};
		static int currentItem{ static_cast<int>(RenderingMode::CombinedGI) }; // TODO(snowapril) : to member-variable
		ImGui::Combo("VCT Rendering Mode", &currentItem, kRenderingModeLabels, sizeof(kRenderingModeLabels) / sizeof(const char*));
		_renderingMode = static_cast<RenderingMode>(currentItem);
		ImGui::Checkbox("Use 32 Cones for Tracing", &_enable32Cones);
		ImGui::SliderFloat("Indirect Diffuse Intensity",	&_indirectDiffuseIntensity,		 0.0f, 30.0f);
		ImGui::SliderFloat("Indirect Specular Intensity",	&_indirectSpecularIntensity,	 0.0f, 10.0f);
		ImGui::SliderFloat("Ambient Occlusion Factor",		&_ambientOcclusionFactor,		 0.0f,  1.0f);
		ImGui::SliderFloat("Occlusion Decay",				&_occlusionDecay,				 0.0f,  5.0f);
		ImGui::SliderFloat("Trace Start Offset",			&_traceStartOffset,				 1.0f, 10.0f);
		ImGui::SliderFloat("Trace Step Factor",				&_minTraceStepFactor,			0.01f,  3.0f);
	}

	OctreeVoxelConeTracing::FramebufferAttachment OctreeVoxelConeTracing::createAttachment(VkExtent3D resolution, VkFormat format, VkImageUsageFlags usage)
	{
		usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		VkImageCreateInfo imageInfo = Image::GetDefaultImageCreateInfo();
		imageInfo.extent		= resolution;
		imageInfo.format		= format;
		imageInfo.usage			= usage;
		imageInfo.imageType		= VK_IMAGE_TYPE_2D;
		// imageInfo.samples	= VK_SAMPLE_COUNT_4_BIT;
		ImagePtr image = std::make_shared<Image>(_device->getMemoryAllocator(), VMA_MEMORY_USAGE_GPU_ONLY, imageInfo);
		
		VkImageViewCreateInfo imageViewInfo = ImageView::GetDefaultImageViewInfo();
		imageViewInfo.format	= format;
		imageViewInfo.viewType	= VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.subresourceRange.aspectMask = aspect;
		ImageViewPtr imageView = std::make_shared<ImageView>(_device, image, imageViewInfo);
		
		return { image, imageView };
	}

	OctreeVoxelConeTracing& OctreeVoxelConeTracing::createAttachments(void)
	{
		const VkExtent3D attachmentExtent = { _resolution.width, _resolution.height, 1 };
		
		// 00. Diffuse contribution
		_attachments.push_back({createAttachment(attachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)});

		// 01. Specular contribution
		_attachments.push_back({createAttachment(attachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)});

		// Create sampler for color attachments
		_colorSampler = std::make_shared<Sampler>(_device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_FILTER_LINEAR, 1.0f);

		// Share render target with other render-passes
		_renderPassManager->put("DiffuseContributionImageView",	 _attachments[0].imageView.get());
		_renderPassManager->put("SpecularContributionImageView", _attachments[1].imageView.get());
		_renderPassManager->put("VCTSampler",				 _colorSampler.get());

#if defined(_DEBUG)
		_debugUtils.setObjectName(_attachments[0].image->getImageHandle(),			"VCT(Diffuse-Contribution)"		 );
		_debugUtils.setObjectName(_attachments[0].imageView->getImageViewHandle(),	"VCT(Diffuse-Contribution) View"	 );
		_debugUtils.setObjectName(_attachments[1].image->getImageHandle(),			"VCT(Specular-Contribution)"		 );
		_debugUtils.setObjectName(_attachments[1].imageView->getImageViewHandle(),	"VCT(Specular-Contribution) View" );
		_debugUtils.setObjectName(_colorSampler->getSamplerHandle(),				"VCT Sampler"				 );
#endif

		return *this;
	}

	OctreeVoxelConeTracing& OctreeVoxelConeTracing::createRenderPass(void)
	{
		assert(_attachments.empty() == false); // snowapril : Attachments must be filled first
		uint32_t attachmentOffset = 0;

		std::vector<VkAttachmentDescription> attachmentDesc;
		std::vector<VkAttachmentReference>	 colorAttachmentRefs;
		attachmentDesc.reserve(_attachments.size());
		colorAttachmentRefs.reserve(_attachments.size());
		for (size_t i = 0; i < _attachments.size(); ++i)
		{
			VkAttachmentDescription tempDesc = {};
			tempDesc.format			= _attachments[i].image->getImageFormat();
			tempDesc.samples		= VK_SAMPLE_COUNT_1_BIT;
			tempDesc.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
			tempDesc.storeOp		= VK_ATTACHMENT_STORE_OP_STORE;
			tempDesc.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			tempDesc.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
			tempDesc.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
			tempDesc.finalLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			attachmentDesc.emplace_back(std::move(tempDesc));
			colorAttachmentRefs.push_back({ attachmentOffset++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		}

		std::vector<VkSubpassDependency> subpassDependencies(2, VkSubpassDependency{});
		subpassDependencies[0].srcSubpass		= VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].dstSubpass		= 0;
		subpassDependencies[0].srcStageMask		= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[0].srcAccessMask	= VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[0].dstStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;
		
		subpassDependencies[1].srcSubpass		= 0;
		subpassDependencies[1].dstSubpass		= VK_SUBPASS_EXTERNAL;
		subpassDependencies[1].srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[1].srcAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[1].dstStageMask		= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[1].dstAccessMask	= VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[1].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;

		VkSubpassDescription subpassDesc = {};
		subpassDesc.colorAttachmentCount	= static_cast<uint32_t>(colorAttachmentRefs.size());
		subpassDesc.pColorAttachments		= colorAttachmentRefs.data();
		subpassDesc.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;

		_renderPass = std::make_shared<RenderPass>();
		assert(_renderPass->initialize(_device, attachmentDesc, subpassDependencies, { subpassDesc }));
		return *this;
	}

	OctreeVoxelConeTracing& OctreeVoxelConeTracing::createFramebuffer(void)
	{
		assert(_attachments.empty() == false && _renderPass != nullptr);
		std::vector<VkImageView> imageViews(_attachments.size());
		for (size_t i = 0; i < _attachments.size(); ++i)
		{
			imageViews[i] = _attachments[i].imageView->getImageViewHandle();
		}
		_framebuffer = std::make_shared<Framebuffer>(_device, imageViews, _renderPass->getHandle(), _resolution);
		return *this;
	}

	OctreeVoxelConeTracing& OctreeVoxelConeTracing::createDescriptors(const BufferPtr& svo)
	{
		// Descriptors for voxel cone tracing
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,		10 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,		 1 },
		};
		_descriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 3, 0);

		_gbufferDescriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_gbufferDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_gbufferDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_gbufferDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_gbufferDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_gbufferDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 4, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_gbufferDescriptorLayout->createDescriptorSetLayout(0);
		
		_gbufferDescriptorSet = std::make_shared<DescriptorSet>(_device, _descriptorPool, _gbufferDescriptorLayout, 1);
		const ImageView* gbufferContents[] = {
			 _renderPassManager->get<ImageView>("DiffuseImageView"),
			 _renderPassManager->get<ImageView>("NormalImageView"),
			 _renderPassManager->get<ImageView>("SpecularImageView"),
			 _renderPassManager->get<ImageView>("EmissionImageView"),
			 _renderPassManager->get<ImageView>("DepthImageView")
		};
		const Sampler*	 gbufferSampler	= _renderPassManager->get<Sampler>("GBufferSampler");
		for (uint32_t i = 0; i < 5; ++i)
		{
			VkDescriptorImageInfo gbufferDescInfo = {};
			if (i == 4)
			{
				gbufferDescInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
			else
			{
				gbufferDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
			gbufferDescInfo.imageView	= gbufferContents[i]->getImageViewHandle();
			gbufferDescInfo.sampler		= gbufferSampler->getSamplerHandle();
			_gbufferDescriptorSet->updateImage({ gbufferDescInfo }, i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		}

		_descriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0);
		_descriptorLayout->createDescriptorSetLayout(0);

		_descriptorSet = std::make_shared<DescriptorSet>(_device, _descriptorPool, _descriptorLayout, 1);
		_descriptorSet->updateStorageBuffer({ svo }, 0, 1);

		_lightDescriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_lightDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		_lightDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_lightDescriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
		_lightDescriptorLayout->createDescriptorSetLayout(0);

		_lightDescriptorSet = std::make_shared<DescriptorSet>(_device, _descriptorPool, _lightDescriptorLayout, 1);
		
		_shadowSampler = std::make_shared<Sampler>(_device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_FILTER_LINEAR, 0.0f);

		DirectionalLight* dirLight = _renderPassManager->get<DirectionalLight>("DirectionalLight");
		VkDescriptorImageInfo shadowMapInfo = {};
		shadowMapInfo.sampler		= _shadowSampler->getSamplerHandle();
		shadowMapInfo.imageView		= dirLight->getShadowMapView()->getImageViewHandle();
		shadowMapInfo.imageLayout	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		_lightDescriptorSet->updateImage({ shadowMapInfo }, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		_lightDescriptorSet->updateUniformBuffer({ dirLight->getLightDescBuffer() }, 1, 1);
		_lightDescriptorSet->updateUniformBuffer({ dirLight->getViewProjectionBuffer() }, 2, 1);

		return *this;
	}

	OctreeVoxelConeTracing& OctreeVoxelConeTracing::createPipeline(const DescriptorSetLayoutPtr& globalDescLayout)
	{
		VkPushConstantRange renderModePush = {};
		renderModePush.stageFlags	= VK_SHADER_STAGE_FRAGMENT_BIT;
		renderModePush.size			= sizeof(VoxelConeTracingDesc);
		renderModePush.offset		= 0;

		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(
			_device,
			{ globalDescLayout, _gbufferDescriptorLayout, _descriptorLayout, _lightDescriptorLayout },
			{ renderModePush }
		);

		PipelineConfig config;
		PipelineConfig::GetDefaultConfig(&config);
		
		config.inputAseemblyInfo.topology			= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		config.rasterizationInfo.cullMode			= VK_CULL_MODE_NONE;
		config.depthStencilInfo.depthTestEnable		= VK_FALSE;
		config.depthStencilInfo.stencilTestEnable	= VK_FALSE;
		config.renderPass							= _renderPass->getHandle();
		config.pipelineLayout						= _pipelineLayout->getLayoutHandle();
		config.dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		config.dynamicStateInfo.dynamicStateCount	= static_cast<uint32_t>(config.dynamicStates.size());
		config.dynamicStateInfo.pDynamicStates		= config.dynamicStates.data();
		config.colorBlendAttachments.clear();
		VkPipelineColorBlendAttachmentState colorBlend = {};
		colorBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
									VK_COLOR_COMPONENT_G_BIT |
									VK_COLOR_COMPONENT_B_BIT |
									VK_COLOR_COMPONENT_A_BIT;
		colorBlend.blendEnable = VK_FALSE;
		config.colorBlendAttachments.resize(_attachments.size(), colorBlend);
		config.colorBlendInfo.attachmentCount = static_cast<uint32_t>(config.colorBlendAttachments.size());
		config.colorBlendInfo.pAttachments = config.colorBlendAttachments.data();

		_pipeline = std::make_shared<GraphicsPipeline>(_device);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_VERTEX_BIT,	"Shaders/voxelConeTracing.vert.spv", nullptr);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/voxelConeTracing_Octree.frag.spv", nullptr);
		_pipeline->createPipeline(&config);
		return *this;
	}
};