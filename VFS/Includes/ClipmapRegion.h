// Author : Jihong Shin (snowapril)

#if !defined(VFS_CLIPMAP_REGION_H)
#define VFS_CLIPMAP_REGION_H

#include <Common/pch.h>

namespace vfs
{
	struct ClipmapRegion
	{
		glm::ivec3 minCorner	{ 0, 0, 0 };
		glm::uvec3 extent		{ 0, 0, 0 };
		float	   voxelSize	{	0.0f  };

		ClipmapRegion() = default;
		explicit ClipmapRegion(glm::ivec3 minCorner_, glm::uvec3 extent_, float voxelSize_)
			: minCorner(minCorner_), extent(extent_), voxelSize(voxelSize_) {};
	};
};

#endif