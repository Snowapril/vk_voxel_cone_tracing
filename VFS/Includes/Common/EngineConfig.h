// Author : Jihong Shin (snowapril)

#if !defined(COMMON_ENGINE_CONFIG_H)
#define COMMON_ENGINE_CONFIG_H

#include <stdint.h>

// TODO(snowapril) : replace hard-coded engine configs in every module with these config
namespace vfs
{
	// RenderEngine Configs
	constexpr const char*	DEFAULT_APP_TITLE				= "Voxel Cone Tracing Global Illumination";
	constexpr int32_t		DEFAULT_WINDOW_WIDTH			= 1280;
	constexpr int32_t		DEFAULT_WINDOW_HEIGHT			= 920;
	constexpr uint32_t		DESCRIPTOR_LAYOUT_GLOBAL		= 0;
	constexpr uint32_t		DESCRIPTOR_LAYOUT_RENDER_PASS	= 1;

	// VoxelEngine Configs
	constexpr uint32_t		DEFAULT_OCTREE_LEVEL		= 6u;
	constexpr uint32_t		MIN_OCTREE_LEVEL			= 1u;
	constexpr uint32_t		MAX_OCTREE_LEVEL			= 10u;
	constexpr uint32_t		MIN_OCTREE_NODE_NUM			= 1000000u;
	constexpr uint32_t		MAX_OCTREE_NODE_NUM			= 500000000u;
	constexpr uint32_t		MIN_OCTREE_BRICK_NUM		= 1125000u;
	constexpr uint32_t		MAX_OCTREE_BRICK_NUM		= 562500000u;

	// Voxel Cone Tracing Configs
	constexpr uint32_t		DEFAULT_VOXEL_FACE_COUNT		= 6;
	constexpr uint32_t		DEFAULT_CLIP_REGION_COUNT		= 6;
	constexpr uint32_t		DEFAULT_VOXEL_BORDER			= 2;
	constexpr uint32_t		DEFAULT_VOXEL_RESOLUTION		= 128;
	constexpr uint32_t		DEFAULT_DOWNSAMPLE_REGION_SIZE	= 10;
	// FluidEngine Configs
	// constexpr 

	// UIEngine Configs
	constexpr uint32_t		MAX_LOG_COUNT				= 256u;

	// Application COnfigs
	constexpr uint32_t		DEFAULT_NUM_FRAMES			= 2u;
}

#endif