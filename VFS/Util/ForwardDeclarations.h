#if !defined(VFS_FORWARD_DECLARATIONS_H)
#define VFS_FORWARD_DECLARATIONS_H

#include <memory>

namespace vfs
{
	class Camera;

	using CameraPtr = std::shared_ptr<Camera>;
};

#endif