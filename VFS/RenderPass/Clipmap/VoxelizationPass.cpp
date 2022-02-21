// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <RenderPass/Clipmap/VoxelizationPass.h>
#include <RenderPass/Clipmap/Voxelizer.h>
#include <RenderPass/RenderPassManager.h>
#include <RenderPass/Clipmap/ClipmapCleaner.h>
#include <RenderPass/Clipmap/BorderWrapper.h>
#include <RenderPass/Clipmap/DownSampler.h>
#include <VulkanFramework/Commands/CommandPool.h>
#include <VulkanFramework/Commands/CommandBuffer.h>
#include <VulkanFramework/Buffers/Buffer.h>
#include <VulkanFramework/Device.h>
#include <VulkanFramework/Descriptors/DescriptorSetLayout.h>
#include <VulkanFramework/Descriptors/DescriptorSet.h>
#include <VulkanFramework/Descriptors/DescriptorPool.h>
#include <VulkanFramework/Images/Image.h>
#include <VulkanFramework/Images/ImageView.h>
#include <VulkanFramework/Images/Sampler.h>
#include <VulkanFramework/RenderPass/Framebuffer.h>
#include <VulkanFramework/RenderPass/RenderPass.h>
#include <VulkanFramework/Pipelines/GraphicsPipeline.h>
#include <VulkanFramework/Pipelines/PipelineLayout.h>
#include <VulkanFramework/Pipelines/PipelineConfig.h>
#include <VulkanFramework/Sync/Fence.h>
#include <VulkanFramework/FrameLayout.h>
#include <VulkanFramework/Queue.h>
#include <VulkanFramework/Utils.h>
#include <Camera.h>
#include <SceneManager.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>

namespace vfs
{
	VoxelizationPass::VoxelizationPass(CommandPoolPtr cmdPool,
									   uint32_t voxelResolution)
		: RenderPassBase(cmdPool), 
		  _voxelResolution(voxelResolution)
	{
		// Do nothing
	}


	VoxelizationPass::VoxelizationPass(CommandPoolPtr cmdPool, uint32_t voxelResolution,
									   uint32_t voxelExtentLevel0, const DescriptorSetLayoutPtr& globalDescLayout)
		: RenderPassBase(cmdPool)
	{
		assert(initialize(voxelResolution, voxelExtentLevel0, globalDescLayout));
	}

	VoxelizationPass::~VoxelizationPass()
	{
		// Do nothing
	}

	bool VoxelizationPass::initialize(uint32_t voxelResolution, uint32_t voxelExtentLevel0, 
									  const DescriptorSetLayoutPtr& globalDescLayout)
	{
		_voxelResolution = voxelResolution;
		
		_voxelizer			= _renderPassManager->get<Voxelizer>("Voxelizer"		);
		_voxelOpacity		= _renderPassManager->get<Image>	("VoxelOpacity"		);
		_voxelOpacityView	= _renderPassManager->get<ImageView>("VoxelOpacityView"	);
		_voxelSampler		= _renderPassManager->get<Sampler>	("VoxelSampler"		);

		createVoxelClipmap(voxelExtentLevel0);
		createDescriptors();
		createPipeline(globalDescLayout);

		return true;
	}

	void VoxelizationPass::onBeginRenderPass(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);

		bool bNeedClearRegion = false;
		if (_fullRevoxelization)
		{
			for (uint32_t i = 0; i < DEFAULT_CLIP_REGION_COUNT; ++i)
			{
				_revoxelizationRegions[i].clear();
				_revoxelizationRegions[i].push_back(_clipmapRegions[i]);
			}
			_fullRevoxelization			= true;  // false to use toroidal addressing
			bNeedClearRegion			= false;
		}
		else
		{
			std::array<BoundingBox<glm::vec3>, DEFAULT_CLIP_REGION_COUNT>* clipRegionBoundingBox =
				_renderPassManager->get< std::array<BoundingBox<glm::vec3>, DEFAULT_CLIP_REGION_COUNT>>("ClipRegionBoundingBox");
			for (uint32_t i = 0; i < DEFAULT_CLIP_REGION_COUNT; ++i)
			{
				_revoxelizationRegions[i].clear();
				fillRevoxelizationRegions(i, clipRegionBoundingBox->at(i));
				bNeedClearRegion |= _revoxelizationRegions[i].empty() == false;
			}
		}
		
		// Clear revoxelization target regions
		{
			DebugUtils::ScopedCmdLabel scope = _debugUtils.scopeLabel(cmdBuffer.getHandle(), "Opacity Clear Regions");
			if (_fullRevoxelization)
			{
				cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					0, {}, {},
					{ _voxelOpacity->generateMemoryBarrier(0, 0, VK_IMAGE_ASPECT_COLOR_BIT,
															VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) }
				);

				VkClearColorValue clearValue;
				clearValue.float32[0] = 0.0f;
				clearValue.float32[1] = 0.0f;
				clearValue.float32[2] = 0.0f;
				clearValue.float32[3] = 0.0f;
				VkImageSubresourceRange imageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
				vkCmdClearColorImage(cmdBuffer.getHandle(), _voxelOpacity->getImageHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					&clearValue, 1, &imageSubresourceRange);

				cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					0, {}, {},
					{ _voxelOpacity->generateMemoryBarrier(0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
															VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL) }
				);
			}
			else
			{
				if (bNeedClearRegion)
				{
					ClipmapCleaner* clipmapCleaner = _renderPassManager->get<ClipmapCleaner>("ClipmapCleaner");
					for (uint32_t clipLevel = 0; clipLevel < DEFAULT_CLIP_REGION_COUNT; ++clipLevel)
					{
						const glm::ivec3 extent = glm::ivec3(_clipmapRegions[clipLevel].extent);
						for (const ClipmapRegion& region : _revoxelizationRegions[clipLevel])
						{
							const glm::ivec3 regionMinCorner = ((region.minCorner % extent) + extent) % extent;
							clipmapCleaner->cmdClearOpacityClipRegion(cmdBuffer, _voxelOpacity, regionMinCorner, region.extent, clipLevel);
						}
					}
				}
				else
				{
					cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
						0, {}, {},
						{ _voxelOpacity->generateMemoryBarrier(0, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
									VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL) }
					);
				}
			}
		}

		_voxelizer->beginRenderPass(frameLayout);
	}

	void VoxelizationPass::onEndRenderPass(const FrameLayout* frameLayout)
	{
		_voxelizer->endRenderPass(frameLayout);
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
		VkImageMemoryBarrier barrier = _voxelOpacity->generateMemoryBarrier(
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		
		cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, {}, {}, { barrier });

		// 1. Down-sampling
		{
			DebugUtils::ScopedCmdLabel scope = _debugUtils.scopeLabel(cmdBuffer.getHandle(), "Opacity DownSampling");
			DownSampler* downSampler = _renderPassManager->get<DownSampler>("DownSampler");
			for (uint32_t i = 1; i < DEFAULT_CLIP_REGION_COUNT; ++i)
			{
				if (_revoxelizationRegions[i].empty() == false)
				{
					downSampler->cmdDownSampleOpacity(cmdBuffer, _voxelOpacity, _clipmapRegions, i);
				}
			}
		}

		// 2. Border wrapping
		{
			DebugUtils::ScopedCmdLabel scope = _debugUtils.scopeLabel(cmdBuffer.getHandle(), "Opacity Border Wrapping");
			BorderWrapper* borderWrapper = _renderPassManager->get<BorderWrapper>("BorderWrapper");
			borderWrapper->cmdWrappingOpacityBorder(cmdBuffer, _voxelOpacity);
		}

		// updateOpacityVoxelSlice();
	}
	
	void VoxelizationPass::onUpdate(const FrameLayout* frameLayout)
	{
		CommandBuffer cmdBuffer(frameLayout->commandBuffer);
		
		// 0. Opacity Voxelization
		{
			DebugUtils::ScopedCmdLabel scope = _debugUtils.scopeLabel(cmdBuffer.getHandle(), "Opacity Voxelization");
			
			cmdBuffer.bindPipeline(_pipeline);
			cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 2, { _descriptorSet }, {});
			SceneManager* sceneManager = _renderPassManager->get<SceneManager>("SceneManager");
			for (uint32_t i = 0; i < DEFAULT_CLIP_REGION_COUNT; ++i)
			{
				for (const ClipmapRegion& region : _revoxelizationRegions[i])
				{
					// voxelize given region
					cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout->getLayoutHandle(), 3, { _voxelizer->getVoxelDescSet(i) }, {});
					_voxelizer->cmdVoxelize(frameLayout->commandBuffer, region, i);
					sceneManager->cmdDraw(frameLayout->commandBuffer, _pipelineLayout, 0);
				}
			}
		}
	}

	void VoxelizationPass::drawGUI(void)
	{
		// Print Voxelization Pass elapsed time
	}

	void VoxelizationPass::drawDebugInfo(void)
	{
		const ImVec2 uv0		 = { 0.0f, 0.0f };
		const ImVec2 uv1		 = { 1.0f, 1.0f };
		const ImVec4 tintColor	 = { 1.0f, 1.0f, 1.0f, 1.0f };
		const ImVec4 borderColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		if (ImGui::TreeNode("Opacity Clipmap"))
		{
			for (uint32_t i = 0; i < 130; i += 5)
			{
				ImGui::Image(_opacitySliceDescSet[  i  ], ImVec2(64.0f, 64.0f), uv0, uv1, tintColor, borderColor);
				ImGui::SameLine();
				ImGui::Image(_opacitySliceDescSet[i + 1], ImVec2(64.0f, 64.0f), uv0, uv1, tintColor, borderColor);
				ImGui::SameLine();
				ImGui::Image(_opacitySliceDescSet[i + 2], ImVec2(64.0f, 64.0f), uv0, uv1, tintColor, borderColor);
				ImGui::SameLine();
				ImGui::Image(_opacitySliceDescSet[i + 3], ImVec2(64.0f, 64.0f), uv0, uv1, tintColor, borderColor);
				ImGui::SameLine();
				ImGui::Image(_opacitySliceDescSet[i + 4], ImVec2(64.0f, 64.0f), uv0, uv1, tintColor, borderColor);
			}
			ImGui::TreePop();
		}
	}

	void VoxelizationPass::createOpacityVoxelSlice(void)
	{
		_opacitySliceSampler = std::make_shared<Sampler>(_device, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FILTER_LINEAR, 0.0f);

		const uint32_t clipWidth  = (_voxelResolution + DEFAULT_VOXEL_BORDER) * DEFAULT_VOXEL_FACE_COUNT;
		const uint32_t clipHeight = (_voxelResolution + DEFAULT_VOXEL_BORDER) * DEFAULT_CLIP_REGION_COUNT;
		for (uint32_t i = 0; i < 130; ++i)
		{
			VkImageCreateInfo imageInfo = Image::GetDefaultImageCreateInfo();
			imageInfo.extent		= { clipWidth, clipHeight, 1};
			imageInfo.format		= VK_FORMAT_R8G8B8A8_UNORM;
			imageInfo.usage			= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageInfo.imageType		= VK_IMAGE_TYPE_2D;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.mipLevels		= 1;
			ImagePtr imageBuffer = std::make_shared<Image>(_device->getMemoryAllocator(), VMA_MEMORY_USAGE_GPU_ONLY, imageInfo);

			_opacitySlice.push_back(std::make_pair(
				imageBuffer,
				std::make_shared<ImageView>(_device, imageBuffer, VK_IMAGE_ASPECT_COLOR_BIT, 1)
			));
			_opacitySliceDescSet.push_back(ImGui_ImplVulkan_AddTexture(
				_opacitySliceSampler->getSamplerHandle(), _opacitySlice[i].second->getImageViewHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			));
		}

		updateOpacityVoxelSlice();
	}

	void VoxelizationPass::updateOpacityVoxelSlice(void)
	{
		CommandPool copyCmdPool(_device, _queue, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

		const uint32_t clipWidth  = (_voxelResolution + DEFAULT_VOXEL_BORDER) * DEFAULT_VOXEL_FACE_COUNT;
		const uint32_t clipHeight = (_voxelResolution + DEFAULT_VOXEL_BORDER) * DEFAULT_CLIP_REGION_COUNT;
		for (uint32_t i = 0; i < 130; ++i)
		{
			ImagePtr& imageBuffer = _opacitySlice[i].first;
			
			CommandBuffer cmdBuffer(copyCmdPool.allocateCommandBuffer());
			cmdBuffer.beginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0, {}, {},
				{ _voxelOpacity->generateMemoryBarrier(0, 0, VK_IMAGE_ASPECT_COLOR_BIT,
														VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) }
			);
			
			cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0, {}, {},
				{ imageBuffer->generateMemoryBarrier(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) }
			);

			VkImageCopy copyRegion = {};
			copyRegion.srcOffset = { 0, 0, static_cast<int32_t>(i) };
			copyRegion.srcSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.srcSubresource.baseArrayLayer	= 0;
			copyRegion.srcSubresource.layerCount		= 1;
			copyRegion.srcSubresource.mipLevel			= 0;
			copyRegion.dstOffset = { 0, 0, 0 };
			copyRegion.dstSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.dstSubresource.baseArrayLayer	= 0;
			copyRegion.dstSubresource.layerCount		= 1;
			copyRegion.dstSubresource.mipLevel			= 0;
			copyRegion.extent = { clipWidth, clipHeight, 1 };

			vkCmdCopyImage(cmdBuffer.getHandle(), _voxelOpacity->getImageHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				imageBuffer->getImageHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0, {}, {},
				{ imageBuffer->generateMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, 0, VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) }
			);
			
			cmdBuffer.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0, {}, {},
				{ _voxelOpacity->generateMemoryBarrier(0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
														VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL) }
			);

			cmdBuffer.endRecord();

			Fence fence(_device, 1, 0);
			_queue->submitCmdBuffer({ cmdBuffer }, &fence);
			fence.waitForAllFences(UINT64_MAX);
		}
	}

	VoxelizationPass& VoxelizationPass::createVoxelClipmap(uint32_t extentLevel0)
	{
		const uint32_t voxelHalfResolution = _voxelResolution >> 1;

		for (size_t i = 0; i < _clipmapRegions.size(); ++i)
		{
			_clipmapRegions[i].minCorner = -glm::ivec3(voxelHalfResolution);
			_clipmapRegions[i].extent = { _voxelResolution, _voxelResolution, _voxelResolution };
			_clipmapRegions[i].voxelSize = (extentLevel0 * (1 << i)) / static_cast<float>(_voxelResolution);
		}

		std::array<BoundingBox<glm::vec3>, DEFAULT_CLIP_REGION_COUNT>* clipRegionBoundingBox =
			_renderPassManager->get< std::array<BoundingBox<glm::vec3>, DEFAULT_CLIP_REGION_COUNT>>("ClipRegionBoundingBox");
		
		for (uint32_t clipmapLevel = 0; clipmapLevel < DEFAULT_CLIP_REGION_COUNT; ++clipmapLevel)
		{
			const glm::ivec3 delta = calculateChangeDelta(clipmapLevel, clipRegionBoundingBox->at(clipmapLevel));
			_clipmapRegions[clipmapLevel].minCorner += delta;
		}

		_renderPassManager->put("ClipmapRegions", &_clipmapRegions);
		return *this;
	}

	VoxelizationPass& VoxelizationPass::createDescriptors(void)
	{
		// Descriptors for voxel textures
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
		};
		_descriptorPool = std::make_shared<DescriptorPool>(_device, poolSizes, 1, 0);

		_descriptorLayout = std::make_shared<DescriptorSetLayout>(_device);
		_descriptorLayout->addBinding(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0);
		_descriptorLayout->createDescriptorSetLayout(0);

		_descriptorSet = std::make_shared<DescriptorSet>(_device, _descriptorPool, _descriptorLayout, 1);

		VkDescriptorImageInfo voxelOpacityInfo = {};
		voxelOpacityInfo.sampler		= _voxelSampler->getSamplerHandle();
		voxelOpacityInfo.imageView		= _voxelOpacityView->getImageViewHandle();
		voxelOpacityInfo.imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
		_descriptorSet->updateImage({ voxelOpacityInfo }, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		return *this;
	}

	VoxelizationPass& VoxelizationPass::createPipeline(const DescriptorSetLayoutPtr& globalDescLayout)
	{
		SceneManager* sceneManager = _renderPassManager->get<SceneManager>("SceneManager");

		_pipelineLayout = std::make_shared<PipelineLayout>();
		_pipelineLayout->initialize(
			_device,
			{ globalDescLayout, sceneManager->getDescriptorLayout(), _descriptorLayout, _voxelizer->getVoxelDescLayout() },
			{ sceneManager->getDefaultPushConstant() }
		);

		PipelineConfig config;
		PipelineConfig::GetDefaultConfig(&config);
		
		const std::vector<VkVertexInputBindingDescription>& bindingDesc		= sceneManager->getVertexInputBindingDesc(0);
		const std::vector<VkVertexInputAttributeDescription>& attribDescs	= sceneManager->getVertexInputAttribDesc(0);
		config.vertexInputInfo.vertexAttributeDescriptionCount	= static_cast<uint32_t>(attribDescs.size());
		config.vertexInputInfo.pVertexAttributeDescriptions		= attribDescs.data();
		config.vertexInputInfo.vertexBindingDescriptionCount	= static_cast<uint32_t>(bindingDesc.size());
		config.vertexInputInfo.pVertexBindingDescriptions		= bindingDesc.data();
		config.inputAseemblyInfo.topology						= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		config.rasterizationInfo.cullMode						= VK_CULL_MODE_NONE;
		config.depthStencilInfo.depthTestEnable					= VK_FALSE;
		config.depthStencilInfo.stencilTestEnable				= VK_FALSE;
		config.renderPass										= _voxelizer->getRenderPass()->getHandle();
		config.pipelineLayout									= _pipelineLayout->getLayoutHandle();
		config.multisampleInfo.rasterizationSamples				= getMaximumSampleCounts(_device);
		if (_device->getDeviceFeature().sampleRateShading == VK_TRUE)
		{
			config.multisampleInfo.sampleShadingEnable = VK_TRUE;
			config.multisampleInfo.minSampleShading = 0.25f;
		}
		else
		{
			config.multisampleInfo.sampleShadingEnable = VK_FALSE;
		}
		config.dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		config.dynamicStateInfo.dynamicStateCount	= static_cast<uint32_t>(config.dynamicStates.size());
		config.dynamicStateInfo.pDynamicStates		= config.dynamicStates.data();
		config.viewportInfo.viewportCount			= 3;
		config.viewportInfo.scissorCount			= 3;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		colorBlendAttachment.colorWriteMask			= 0;
		colorBlendAttachment.blendEnable			= VK_FALSE;
		config.colorBlendInfo.attachmentCount		= 1;
		config.colorBlendInfo.pAttachments			= &colorBlendAttachment;

		_pipeline = std::make_shared<GraphicsPipeline>(_device);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_VERTEX_BIT,	"Shaders/msaaVoxelizer.vert.spv", nullptr);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_GEOMETRY_BIT, "Shaders/msaaVoxelizer.geom.spv", nullptr);
		_pipeline->attachShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/msaaVoxelizer.frag.spv", nullptr);
		_pipeline->createPipeline(&config);

		return *this;
	}

	glm::ivec3 VoxelizationPass::calculateChangeDelta(const uint32_t clipLevel, const BoundingBox<glm::vec3>& cameraBB)
	{
		assert(clipLevel < DEFAULT_CLIP_REGION_COUNT);
		const ClipmapRegion& region = _clipmapRegions[clipLevel];
		const float voxelSize = region.voxelSize;

		glm::vec3 deltaW = cameraBB.getMinCorner() - (glm::vec3(region.minCorner) * voxelSize);
		const float minChange = voxelSize * _clipMinChange[clipLevel];

		return glm::ivec3(glm::trunc(deltaW / minChange)) * _clipMinChange[clipLevel];
	}

	void VoxelizationPass::fillRevoxelizationRegions(const uint32_t clipLevel, const BoundingBox<glm::vec3>& boundingBox)
	{
		assert(clipLevel < DEFAULT_CLIP_REGION_COUNT);
		ClipmapRegion& clipmap = _clipmapRegions[clipLevel];

		const glm::ivec3 delta	  = calculateChangeDelta(clipLevel, boundingBox);
		const glm::ivec3 absDelta = glm::abs(delta);

		clipmap.minCorner += delta;

		// If Full revoxelization needed
		if (glm::any(glm::greaterThanEqual(absDelta, glm::ivec3(clipmap.extent))))
		{
			_revoxelizationRegions[clipLevel].push_back(clipmap);
			return;
		}
		// Otherwise
		if (absDelta.x >= _clipMinChange[clipLevel])
		{
			glm::ivec3 newExtent = glm::ivec3(absDelta.x, clipmap.extent.y, clipmap.extent.z);
			_revoxelizationRegions[clipLevel].emplace_back(
				delta.x > 0 ? ((clipmap.minCorner + glm::ivec3(clipmap.extent)) - newExtent) : (clipmap.minCorner),
				newExtent,
				clipmap.voxelSize
			);
		}
		if (absDelta.y >= _clipMinChange[clipLevel])
		{
			glm::ivec3 newExtent = glm::ivec3(clipmap.extent.x, absDelta.y, clipmap.extent.z);
			_revoxelizationRegions[clipLevel].emplace_back(
				delta.y > 0 ? ((clipmap.minCorner + glm::ivec3(clipmap.extent)) - newExtent) : (clipmap.minCorner),
				newExtent,
				clipmap.voxelSize
			);
		}
		if (absDelta.z >= _clipMinChange[clipLevel])
		{
			glm::ivec3 newExtent = glm::ivec3(clipmap.extent.x, clipmap.extent.y, absDelta.z);
			_revoxelizationRegions[clipLevel].emplace_back(
				delta.z > 0 ? ((clipmap.minCorner + glm::ivec3(clipmap.extent)) - newExtent) : (clipmap.minCorner),
				newExtent,
				clipmap.voxelSize
			);
		}
	}
};