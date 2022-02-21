// Author : Jihong Shin (snowapril)

#if !defined(VFS_CLIP_MAP_VIEWER_H)
#define VFS_CLIP_MAP_VIEWER_H

#include <pch.h>

namespace vfs
{
	class ClipmapVisualizer : NonCopyable
	{
	public:
		explicit ClipmapVisualizer(DevicePtr device);
				~ClipmapVisualizer();

	private:
		DevicePtr _device { nullptr };
	};
};

#endif