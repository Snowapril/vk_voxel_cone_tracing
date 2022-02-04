// Author : Jihong Shin (snowapril)

#if !defined(COMMON_GLTF_LOADER_H)
#define COMMON_GLTF_LOADER_H

#include <Common/pch.h>
#include <Common/VertexFormat.h>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <functional>
#include <unordered_map>

namespace vfs
{
	//! KHR extension list (https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos)
	#define KHR_DARCO_MESH_EXTENSION_NAME "KHR_draco_mesh_compression"
	#define KHR_LIGHTS_PUNCTUAL_EXTENSION_NAME "KHR_lights_punctual"
	#define KHR_MATERIALS_CLEARCOAT_EXTENSION_NAME "KHR_materials_clearcoat"
	#define KHR_MATERIALS_PBR_SPECULAR_GLOSSINESS_EXTENSION_NAME "KHR_materials_pbrSpecularGlossiness"
	#define KHR_MATERIALS_SHEEN_EXTENSION_NAME "KHR_materials_sheen"
	#define KHR_MATERIALS_TRANSMISSION_EXTENSION_NAME "KHR_materials_transmission"
	#define KHR_MATERIALS_UNLIT_EXTENSION_NAME "KHR_materials_unlit"
	#define KHR_MATERIALS_VARIANTS_EXTENSION_NAME "KHR_materials_variants"
	#define KHR_MESH_QUANTIZATION_EXTENSION_NAME "KHR_mesh_quantization"
	#define KHR_TEXTURE_TRANSFORM_EXTENSION_NAME "KHR_texture_transform"

	struct GLTFImage
	{
		std::string			 name;
		uint32_t			 width	{ 0 };
		uint32_t			 height	{ 0 };
		std::vector<uint8_t> data;

		explicit GLTFImage(std::string&& name, uint32_t width, 
						   uint32_t height, std::vector<uint8_t>&& data);
	};

	class GLTFLoader : NonCopyable
	{
	public:
		explicit GLTFLoader() = default;
		virtual ~GLTFLoader() = default;

	public:
		bool loadScene(const char* filename, VertexFormat format);

	protected:
		// Material model from gltf official
		// https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#reference-material
		struct GLTFMaterial
		{
			glm::vec4   baseColorFactor			{ 1.0f, 1.0f, 1.0f, 1.0f };
			int			baseColorTexture		{ -1 };
			float		metallicFactor			{ 1.0f };
			float		roughnessFactor			{ 1.0f };
			int			metallicRoughnessTexture{ -1 };

			int			emissiveTexture			{ -1 };
			glm::vec3	emissiveFactor			{ 0.0f, 0.0f, 0.0f };
			int			alphaMode				{ 0 }; // snowapril : OPAQUE(0), MASK(1), BLEND(2)
			float		alphaCutoff				{ 0.5f };
			int			doubleSided				{ 0 };

			int			normalTexture			{ -1 };
			float		normalTextureScale		{ 1.0f };
			int			occlusionTexture		{ -1 };
			float		occlusionTextureStrength{ 1.0f };
		};

		struct GLTFNode
		{
			glm::mat4 world{ 1.0f };
			glm::mat4 local{ 1.0f };
			glm::vec3 translation{ 0.0f };
			glm::vec3 scale{ 1.0f };
			glm::quat rotation{ 0.0f, 0.0f, 0.0f, 0.0f };
			std::vector<unsigned int> primMeshes;
			std::vector<int> childNodes;
			int parentNode{ -1 };
			int nodeIndex{ 0 };
		};

		struct GLTFPrimMesh
		{
			unsigned int firstIndex{ 0 };
			unsigned int indexCount{ 0 };
			unsigned int vertexOffset{ 0 };
			unsigned int vertexCount{ 0 };
			int materialIndex{ 0 };

			glm::vec3 min{ 0.0f, 0.0f, 0.0f };
			glm::vec3 max{ 0.0f, 0.0f, 0.0f };
			std::string name;
		};

		struct GLTFCamera
		{
			glm::mat4 world{ 1.0f };
			glm::vec3 eye{ 0.0f };
			glm::vec3 center{ 0.0f };
			glm::vec3 up{ 0.0f, 1.0f, 0.0f };

			tinygltf::Camera camera;
		};

		struct GLTFLight
		{
			glm::mat4 world{ 1.0f };
			tinygltf::Light light;
		};

		struct SceneDimension
		{
			glm::vec3 min = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
			glm::vec3 max = { std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min() };
			glm::vec3 size = { 0.0f, 0.0f, 0.0f };
			glm::vec3 center = { 0.0f, 0.0f, 0.0f };
			float radius{ 0.0f };
		};

		std::vector<GLTFMaterial> _sceneMaterials;
		std::vector<GLTFNode> _sceneNodes;
		std::vector<GLTFPrimMesh> _scenePrimMeshes;
		std::vector<GLTFCamera> _sceneCameras;
		std::vector<GLTFLight> _sceneLights;

		std::vector<glm::vec3> _positions;
		std::vector<glm::vec3> _normals;
		std::vector<glm::vec4> _tangents;
		std::vector<glm::vec4> _colors;
		std::vector<glm::vec2> _texCoords;
		std::vector<unsigned int> _indices;

		std::vector<GLTFImage> _images;

		SceneDimension _sceneDim;

		//! Release scene source datum
		void releaseSourceData();
	private:
		static bool LoadModel(tinygltf::Model* model, const char* filename);
		template <typename Type>
		static bool GetAttributes(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<Type>& attributes, const char* name);
		static glm::mat4 GetLocalMatrix(const GLTFNode& node);
		void importMaterials(const tinygltf::Model& model);
		void processMesh(const tinygltf::Model& model, const tinygltf::Primitive& mesh, VertexFormat format, const std::string& name);
		void processNode(const tinygltf::Model& model, int nodeIdx, int parentIndex);
		void updateNode(int nodeIndex);
		void calculateSceneDimension();
		void computeCamera();
		template <typename Type>
		static std::vector<Type> GetVector(const tinygltf::Value& value);
		template <typename Type>
		static void GetValue(const tinygltf::Value& value, const char* name, Type& val);
		static void GetTextureID(const tinygltf::Value& value, const char* name, int& id);

		std::unordered_map<unsigned int, std::vector<unsigned int>> _meshToPrimMap;
		std::vector<unsigned int>	_u32Buffer;
		std::vector<unsigned short>	_u16Buffer;
		std::vector<unsigned char>	_u8Buffer;
	};
}

#include <Common/GLTFLoader-Impl.hpp>
#endif