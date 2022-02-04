// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <Common/Utils.h>

namespace vfs
{
	void getDirectory(const char* filePath, OUT char* dir, size_t length)
	{
		const size_t pathLength = strlen(filePath);
		assert(length >= pathLength);

		size_t separatorLoc = 0;
		for (size_t i = pathLength - 1; i > 0; --i)
		{
			if (filePath[i] == '/' || filePath[i] == '\\')
			{
				separatorLoc = i;
			}
		}

		if (separatorLoc != 0)
		{
			memcpy(dir, filePath, separatorLoc + 1);
			dir[separatorLoc + 1] = '\0';
		}
		else
		{
			strcpy(dir, ".");
		}
	}
}

