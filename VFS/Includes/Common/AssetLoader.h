// Author : Jihong Shin (snowapril)

#if !defined(COMMON_ASSET_LOADER)
#define COMMON_ASSET_LOADER

#include <Common/pch.h>
#include <Common/VertexFormat.h>
#include <string>

namespace vfs
{
	struct Mesh
	{
		std::vector<glm::vec3> positions;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> texCoords;
		std::vector<glm::vec4> tangents;
		std::string diffuseTexPaths;
		std::string ambientTexPaths;
		std::string specularTexPaths;
		std::string emissiveTexPaths;
		std::string opacityTexPaths;
		std::string normalTexPaths;
		glm::vec3 diffuseColor;
		glm::vec3 specularColor;
		glm::vec3 emissiveColor;
		float shininess{ 0.0f };
		float opacity{ 0.0f };
	};

	class AssetLoader
	{
	public:
		AssetLoader() = delete;

	public:
		static bool LoadObjFile(const char*					objPath, 
								OUT std::vector<Mesh>*		meshes,
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