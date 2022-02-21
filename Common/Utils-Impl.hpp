// Author : Jihong Shin (snowapril)

#if !defined(COMMON_UTILS_IMPL_H)
#define COMMON_UTILS_IMPL_H

namespace vfs
{
	template <typename Type>
	Type min(Type arg1, Type arg2)
	{
		return arg1 > arg2 ? arg2 : arg1;
	}

	template <typename Type>
	Type max(Type arg1, Type arg2)
	{
		return arg1 > arg2 ? arg1 : arg2;
	}

	template <typename Type>
	Type clamp(Type value, Type minValue, Type maxValue)
	{
		return vfs::max(vfs::min(value, maxValue), minValue);
	}
}

#endif