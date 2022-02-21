// Author : Jihong Shin (snowapril)

#if !defined(COMMON_UTILS_H)
#define COMMON_UTILS_H

namespace vfs
{
	template <typename Type>
	Type min(Type arg1, Type arg2);

	template <typename Type>
	Type max(Type arg1, Type arg2);

	template <typename Type>
	Type clamp(Type value, Type minValue, Type maxValue);

	// Get directory path of given file included
	void getDirectory(const char* filePath, char* dir, size_t length);
}

#include <Common/Utils-Impl.hpp>

#endif