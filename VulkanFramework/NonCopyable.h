// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_NON_COPYABLE_H)
#define VULKAN_FRAMEWORK_NON_COPYABLE_H

namespace vfs
{
	class NonCopyable
	{
	public:
		NonCopyable() = default;
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator=(const NonCopyable&) = delete;
	};
}

#endif