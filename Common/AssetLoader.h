// Author : Jihong Shin (snowapril)

#if !defined(COMMON_ASSET_LOADER)
#define COMMON_ASSET_LOADER

#include <Common/pch.h>
#include <Common/VertexFormat.h>

namespace vfs
{
	class AssetLoader
	{
	public:
		AssetLoader() = delete;

	public:
		static bool LoadObjFile(const char*					objPath, 
								OUT std::vector<float>*		vertices, 
								OUT std::vector<uint32_t>*	indices,
								OUT glm::vec3*				minCorner, 
								OUT glm::vec3*				maxCorner, 
								VertexFormat				format);

		static bool LoadImageU8(const char*		imagePath, 
								OUT uint8_t**	data,
								OUT int*		width, 
								OUT int*		height, 
								OUT int*		numChannels);

		static void FreeImage(void* data);
	};
}

#endif