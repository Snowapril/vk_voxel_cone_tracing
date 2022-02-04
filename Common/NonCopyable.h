// Author : Jihong Shin (snowapril)

#if !defined(COMMON_NON_COPYABLE)
#define COMMON_NON_COPYABLE

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