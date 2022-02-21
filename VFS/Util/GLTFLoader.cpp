// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <Util/GLTFLoader.h>
#include <Common/Logger.h>
#include <unordered_set>
#include <cassert>


#pragma warning (push)

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define TINYGLTF_USE_CPP14
#pragma warning(disable:4018)
#pragma warning(disable:4100)
#pragma warning(disable:4127)
#pragma warning(disable:4244)
#pragma warning(disable:4267)
#pragma warning(disable:4296)
#pragma warning(disable:4804)
#pragma warning(disable:4946)
#pragma warning(disable:6001)
#pragma warning(disable:6262)
#pragma warning(disable:6385)
#pragma warning(disable:26495)
#pragma warning(disable:28020)
#pragma warning(disable:28182)

#include <tinygltf/tiny_gltf.h>

#pragma warning (pop)

namespace vfs 
{
	GLTFImage::GLTFImage(std::string&& name_, uint32_t width_,
						 uint32_t height_, std::vector<uint8_t>&& data_)
		: name(std::move(name_)), width(width_), height(height_), data(std::move(data_))
	{ 
	}

	bool GLTFLoader::loadScene(const char* filename, VertexFormat format)
	{
		assert(static_cast<int>(format & VertexFormat::Position3) && "Scene model must contain Position attribute");

		tinygltf::Model model;
		if (!LoadModel(&model, filename))
			return false;

		uint32_t numVertices{ 0 }, numIndices{ 0 }, primCount{ 0 }, meshCount{ 0 };
		for (const auto& mesh : model.meshes)
		{
			std::vector<uint32_t> vPrim;
			for (const auto& prim : mesh.primitives)
			{
				if (prim.mode != TINYGLTF_MODE_TRIANGLES)
					continue;

				const auto& posAccessor = model.accessors[prim.attributes.find("POSITION")->second];
				numVertices += static_cast<uint32_t>(posAccessor.count);
				if (prim.indices > -1)
				{
					const auto& indexAccessor = model.accessors[prim.indices];
					numIndices += static_cast<uint32_t>(indexAccessor.count);
				}
				else
				{
					numIndices += static_cast<uint32_t>(posAccessor.count);
				}
				vPrim.push_back(primCount++);
			}
			_meshToPrimMap[meshCount++] = std::move(vPrim);
		}

		_positions.reserve(numVertices);
		_indices.reserve(numIndices);
		if (static_cast<int>(format & VertexFormat::Normal3))
			_normals.reserve(numVertices);
		if (static_cast<int>(format & VertexFormat::Tangent4))
			_tangents.reserve(numVertices);
		if (static_cast<int>(format & VertexFormat::Color4))
			_colors.reserve(numVertices);
		if (static_cast<int>(format & VertexFormat::TexCoord2))
			_texCoords.reserve(numVertices);

		// Convert all mesh/primitves+ to a single primitive per mesh.
		for (const auto& mesh : model.meshes)
		{
			for (const auto& prim : mesh.primitives)
			{
				processMesh(model, prim, format, mesh.name);
			}
		}

		// Transforming the scene hierarchy to a flat list.
		int defaultScene = model.defaultScene > -1 ? model.defaultScene : 0;
		const auto& scene = model.scenes[defaultScene];
		for (auto nodeIdx : scene.nodes)
		{
			processNode(model, nodeIdx, -1);
		}

		// Compute scene dimension
		calculateSceneDimension();
		computeCamera();

		// Clear all temporal resources.
		_meshToPrimMap.clear();
		_u8Buffer.clear();
		_u16Buffer.clear();
		_u32Buffer.clear();

		// Import materials from the model
		importMaterials(model);

		// Finally import images from the model
		for (tinygltf::Texture& texture : model.textures)
		{
			tinygltf::Image& image = model.images[texture.source];
			_images.emplace_back(std::move(image.name), image.width, image.height, std::move(image.image));
		}

		return true;
	}

	void GLTFLoader::processMesh(const tinygltf::Model& model, const tinygltf::Primitive& mesh, VertexFormat format, const std::string& name)
	{
		GLTFPrimMesh resultMesh;
		resultMesh.name = name;
		resultMesh.materialIndex = mesh.material < 0 ? 0 : mesh.material;
		resultMesh.vertexOffset = static_cast<uint32_t>(_positions.size());
		resultMesh.firstIndex = static_cast<uint32_t>(_indices.size());

		// Only triangles supported.
		if (mesh.mode != TINYGLTF_MODE_TRIANGLES)
			return;

		// Indices
		if (mesh.indices > -1)
		{
			const tinygltf::Accessor& indexAccessor = model.accessors[mesh.indices];
			const tinygltf::BufferView& bufferView = model.bufferViews[indexAccessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

			resultMesh.indexCount = static_cast<uint32_t>(indexAccessor.count);
			switch (indexAccessor.componentType)
			{
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
				_u32Buffer.resize(indexAccessor.count);
				std::memcpy(&_u32Buffer[0], &buffer.data[indexAccessor.byteOffset + bufferView.byteOffset], indexAccessor.count * sizeof(uint32_t));
				_indices.insert(_indices.end(), std::make_move_iterator(_u32Buffer.begin()), std::make_move_iterator(_u32Buffer.end()));
				break;
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
				_u16Buffer.resize(indexAccessor.count);
				std::memcpy(&_u16Buffer[0], &buffer.data[indexAccessor.byteOffset + bufferView.byteOffset], indexAccessor.count * sizeof(unsigned short));
				_indices.insert(_indices.end(), std::make_move_iterator(_u16Buffer.begin()), std::make_move_iterator(_u16Buffer.end()));
				break;
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
				_u8Buffer.resize(indexAccessor.count);
				std::memcpy(&_u8Buffer[0], &buffer.data[indexAccessor.byteOffset + bufferView.byteOffset], indexAccessor.count * sizeof(unsigned char));
				_indices.insert(_indices.end(), std::make_move_iterator(_u8Buffer.begin()), std::make_move_iterator(_u8Buffer.end()));
				break;
			default:
				VFS_ERROR << "Unknown index component type : " << indexAccessor.componentType << " is not supported";
				return;
			}
		}
		else
		{
			// Primitive without indices, creating them
			const auto& accessor = model.accessors[mesh.attributes.find("POSITION")->second];
			for (uint32_t i = 0; i < accessor.count; ++i)
				_indices.push_back(i);
			resultMesh.indexCount = static_cast<uint32_t>(accessor.count);
		}

		// POSITION
		{
			bool result = GetAttributes<glm::vec3>(model, mesh, _positions, "POSITION");

			// Keeping the size of this primitive (spec says this is required information)
			const auto& accessor = model.accessors[mesh.attributes.find("POSITION")->second];
			resultMesh.vertexCount = static_cast<uint32_t>(accessor.count);
			if (accessor.minValues.empty() == false)
				resultMesh.min = glm::vec3(accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]);
			if (accessor.maxValues.empty() == false)
				resultMesh.max = glm::vec3(accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]);
		}

		// NORMAL
		if (static_cast<int>(format & VertexFormat::Normal3))
		{
			if (!GetAttributes<glm::vec3>(model, mesh, _normals, "NORMAL"))
			{
				// You need to compute the normals
				std::vector<glm::vec3> meshNormals(resultMesh.vertexCount, glm::vec3(0.0f));
				for (size_t i = 0; i < resultMesh.indexCount; i += 3)
				{
					uint32_t idx0 = _indices[resultMesh.firstIndex + i + 0];
					uint32_t idx1 = _indices[resultMesh.firstIndex + i + 1];
					uint32_t idx2 = _indices[resultMesh.firstIndex + i + 2];
					const auto& pos0 = _positions[resultMesh.vertexOffset + idx0];
					const auto& pos1 = _positions[resultMesh.vertexOffset + idx1];
					const auto& pos2 = _positions[resultMesh.vertexOffset + idx2];
					const auto edge0 = glm::normalize(pos1 - pos0);
					const auto edge1 = glm::normalize(pos2 - pos0);
					const auto n = glm::normalize(glm::cross(edge0, edge1));
					meshNormals[idx0] += n;
					meshNormals[idx1] += n;
					meshNormals[idx2] += n;
				}
				_normals.insert(_normals.end(), std::make_move_iterator(meshNormals.begin()), std::make_move_iterator(meshNormals.end()));
			}
		}

		// TEXCOORD2
		if (static_cast<int>(format & VertexFormat::TexCoord2))
		{
			if (!GetAttributes<glm::vec2>(model, mesh, _texCoords, "TEXCOORD_0"))
			{
				// CubeMap projection
				for (uint32_t i = 0; i < resultMesh.vertexCount; ++i)
				{
					const auto& pos = _positions[resultMesh.vertexOffset + i];
					float absX = std::fabs(pos.x);
					float absY = std::fabs(pos.y);
					float absZ = std::fabs(pos.z);

					int isXPositive = pos.x > 0.0f ? 1 : 0;
					int isYPositive = pos.y > 0.0f ? 1 : 0;
					int isZPositive = pos.z > 0.0f ? 1 : 0;

					float mapAxis{ 0.0f }, uc{ 0.0f }, vc{ 0.0f };
					// Positive X
					if (isXPositive && absX >= absY && absX >= absZ)
					{
						// u(0~1) goes from +z to -z
						// v(0~1) goes from -y to +y
						mapAxis = absX;
						uc = -pos.z;
						vc = pos.y;
					}
					// Negative X
					if (!isXPositive && absX >= absY && absX >= absZ)
					{
						// u(0~1) goes from -z to +z
						// v(0~1) goes from -y to +y
						mapAxis = absX;
						uc = pos.z;
						vc = pos.y;
					}
					// Positive Y
					if (isYPositive && absY >= absX && absY >= absZ)
					{
						// u(0~1) goes from -x to +x
						// v(0~1) goes from +z to -z
						mapAxis = absY;
						uc = pos.x;
						vc = -pos.z;
					}
					// Negative Y
					if (!isYPositive && absY >= absX && absY >= absZ)
					{
						// u(0~1) goes from -x to +x
						// v(0~1) goes from -z to +z
						mapAxis = absY;
						uc = pos.x;
						vc = pos.z;
					}
					// Positive Z
					if (isZPositive && absY >= absX && absY >= absZ)
					{
						// u(0~1) goes from -x to +x
						// v(0~1) goes from -y to +y
						mapAxis = absZ;
						uc = pos.x;
						vc = pos.y;
					}
					// Negative Z
					if (!isZPositive && absZ >= absX && absZ >= absY)
					{
						// u(0~1) goes from +x to -x
						// v(0~1) goes from -y to +y
						mapAxis = absZ;
						uc = -pos.x;
						vc = pos.y;
					}

					// Convert range from (-1~1) into (0~1)
					float u = (uc / mapAxis + 1.0f) * 0.5f;
					float v = (vc / mapAxis + 1.0f) * 0.5f;

					_texCoords.push_back(glm::vec2(u, v));
				}
			}
		}

		// TANGENT
		if (static_cast<int>(format & VertexFormat::Tangent4))
		{
			if (!GetAttributes(model, mesh, _tangents, "TANGENT"))
			{
				// Implementation in "Foundations of Game Engine Development : Volume2 Rendering"
				std::vector<glm::vec3> tangents(resultMesh.vertexCount, glm::vec3(0.0f));
				std::vector<glm::vec3> bitangents(resultMesh.vertexCount, glm::vec3(0.0f));
				for (size_t i = 0; i < resultMesh.indexCount; i += 3)
				{
					// Local index
					uint32_t idx0 = _indices[resultMesh.firstIndex + i + 0];
					uint32_t idx1 = _indices[resultMesh.firstIndex + i + 1];
					uint32_t idx2 = _indices[resultMesh.firstIndex + i + 2];
					// Global index
					uint32_t gidx0 = idx0 + resultMesh.vertexOffset;
					uint32_t gidx1 = idx1 + resultMesh.vertexOffset;
					uint32_t gidx2 = idx2 + resultMesh.vertexOffset;

					const auto& pos0 = _positions[gidx0];
					const auto& pos1 = _positions[gidx1];
					const auto& pos2 = _positions[gidx2];

					const auto& uv0 = _texCoords[gidx0];
					const auto& uv1 = _texCoords[gidx1];
					const auto& uv2 = _texCoords[gidx2];

					glm::vec3 e1 = pos1 - pos0, e2 = pos2 - pos0;
					float x1 = uv1.x - uv0.x, x2 = uv2.x - uv0.x;
					float y1 = uv1.y - uv0.y, y2 = uv2.y - uv0.y;

					const float r = 1.0f / (x1 * y2 - x2 * y1);
					glm::vec3 tangent = (e1 * y2 - e2 * y1) * r;
					glm::vec3 bitangent = (e2 * x1 - e1 * x2) * r;

					// In case of degenerated UV coordinates
					if (x1 == 0 || x2 == 0 || y1 == 0 || y2 == 0)
					{
						const auto& nrm0 = _normals[gidx0];
						const auto& nrm1 = _normals[gidx1];
						const auto& nrm2 = _normals[gidx2];
						const auto N = (nrm0 + nrm1 + nrm2) / glm::vec3(3.0f);

						if (std::abs(N.x) > std::abs(N.y))
							tangent = glm::vec3(N.z, 0, -N.x) / std::sqrt(N.x * N.x + N.z * N.z);
						else
							tangent = glm::vec3(0, -N.z, N.y) / std::sqrt(N.y * N.y + N.z * N.z);
						bitangent = glm::cross(N, tangent);
					}

					tangents[idx0] += tangent;
					tangents[idx1] += tangent;
					tangents[idx2] += tangent;
					bitangents[idx0] += bitangent;
					bitangents[idx1] += bitangent;
					bitangents[idx2] += bitangent;
				}

				for (uint32_t i = 0; i < resultMesh.vertexCount; ++i)
				{
					const auto& n = _normals[resultMesh.vertexOffset + i];
					const auto& t = tangents[i];
					const auto& b = bitangents[i];

					// Gram schmidt orthogonalize
					glm::vec3 tangent = glm::normalize(t - n * glm::vec3(glm::dot(n, t)));
					// Calculate the handedness
					float handedness = (glm::dot(glm::cross(t, b), n) > 0.0f) ? 1.0f : -1.0f;
					_tangents.emplace_back(tangent.x, tangent.y, tangent.z, handedness);
				}
			}
		}

		// COLOR
		if (static_cast<int>(format & VertexFormat::Color4))
		{
			if (!GetAttributes(model, mesh, _colors, "COLOR_0"))
			{
				_colors.insert(_colors.end(), resultMesh.vertexCount, glm::vec4(1.0f));
			}
		}

		_scenePrimMeshes.emplace_back(resultMesh);
	}

	bool GLTFLoader::LoadModel(tinygltf::Model* model, const char* filename)
	{
		tinygltf::TinyGLTF loader;
		std::string err, warn;

		bool res = loader.LoadBinaryFromFile(model, &err, &warn, filename);
		if (!res)
		{
			res = loader.LoadASCIIFromFile(model, &err, &warn, filename);
		}

		return res;
	}

	void GLTFLoader::processNode(const tinygltf::Model& model, int nodeIdx, int parentIndex)
	{
		const auto& node = model.nodes[nodeIdx];

		GLTFNode newNode;
		// Gets transformation info from the given node
		if (node.translation.empty() == false)
		{
			newNode.translation = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
		}
		if (node.scale.empty() == false)
		{
			newNode.scale = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
		}
		if (node.rotation.empty() == false)
		{
			newNode.rotation = glm::quat(
				static_cast<float>(node.rotation[3]),
				static_cast<float>(node.rotation[0]),
				static_cast<float>(node.rotation[1]),
				static_cast<float>(node.rotation[2])
			);
		}
		if (node.matrix.empty() == false)
		{
			float* nodeMatPtr = glm::value_ptr(newNode.local);
			for (int i = 0; i < 16; ++i)
				nodeMatPtr[i] = static_cast<float>(node.matrix[i]);
		}

		// Calculate world matrix
		glm::mat4 worldMat = (parentIndex != -1 ? _sceneNodes[parentIndex].world : glm::mat4(1.0f)) * GetLocalMatrix(newNode);

		if (node.camera > -1)
		{
			GLTFCamera camera;
			camera.world = worldMat;
			camera.camera = model.cameras[model.nodes[nodeIdx].camera];

			// If the node has the Iray extension, extract the camera information
			if (node.extensions.find("NV_attributes_iray") != node.extensions.end())
			{
				auto& iray_ext = node.extensions.find("NV_attributes_iray")->second;
				auto& attributes = iray_ext.Get("attributes");
				for (size_t idx = 0; idx < attributes.ArrayLen(); ++idx)
				{
					auto& attrib = attributes.Get(static_cast<int>(idx));
					std::string attName = attrib.Get("name").Get<std::string>();
					auto& attValue = attrib.Get("value");
					if (attValue.IsArray())
					{
						auto vec = GetVector<float>(attValue);
						if (attName == "ivew:position")
							camera.eye = { vec[0], vec[1], vec[2] };
						else if (attName == "iview:interest")
							camera.center = { vec[0], vec[1], vec[2] };
						else if (attName == "iview:up")
							camera.up = { vec[0], vec[1], vec[2] };
					}
				}
			}

			_sceneCameras.emplace_back(camera);
		}
		else if (node.extensions.find(KHR_LIGHTS_PUNCTUAL_EXTENSION_NAME) != node.extensions.end())
		{
			GLTFLight light;
			const auto& ext = node.extensions.find(KHR_LIGHTS_PUNCTUAL_EXTENSION_NAME)->second;
			auto lightIdx = ext.Get("light").GetNumberAsInt();
			light.light = model.lights[lightIdx];
			light.world = worldMat;
			_sceneLights.emplace_back(light);
		}
		else
		{
			if (node.mesh > -1)
				newNode.primMeshes = std::move(_meshToPrimMap[node.mesh]);

			newNode.world = std::move(worldMat);
			newNode.nodeIndex = nodeIdx;
			newNode.parentNode = parentIndex;

			// Push newnode to both linear scene node array and parent child array
			const int newNodeIndex = static_cast<int>(_sceneNodes.size());
			_sceneNodes.emplace_back(std::move(newNode));
			if (parentIndex != -1)
				_sceneNodes[parentIndex].childNodes.push_back(newNodeIndex);

			// Call ProcessNode recursively to the childs of this newNode
			for (auto child : node.children)
				processNode(model, child, newNodeIndex);
		}
	}

	void GLTFLoader::updateNode(int nodeIndex)
	{
		auto& node = _sceneNodes[nodeIndex];
		if (!node.primMeshes.empty())
		{
			node.world = GetLocalMatrix(node);
			int parentNode = node.parentNode;
			while (parentNode != -1)
			{
				node.world = GetLocalMatrix(_sceneNodes[parentNode]) * node.world;
				parentNode = _sceneNodes[parentNode].parentNode;
			}
		}

		for (int child : node.childNodes)
			updateNode(child);
	}

	glm::mat4 GLTFLoader::GetLocalMatrix(const GLTFNode& node)
	{
		return glm::translate(glm::mat4(1.0f), node.translation) *
			glm::toMat4(node.rotation) *
			glm::scale(glm::mat4(1.0f), node.scale) *
			node.local;
	}

	void GLTFLoader::calculateSceneDimension()
	{
		auto bbMin = glm::vec3(std::numeric_limits<float>::max());
		auto bbMax = glm::vec3(std::numeric_limits<float>::min());
		for (const auto& node : _sceneNodes)
		{
			for (uint32_t meshIdx : node.primMeshes)
			{
				const auto& mesh = _scenePrimMeshes[meshIdx];

				auto localMin = node.world * glm::vec4(mesh.min, 1.0f);
				auto localMax = node.world * glm::vec4(mesh.max, 1.0f);

				bbMin = { std::min(bbMin.x, localMin.x), std::min(bbMin.y, localMin.y), std::min(bbMin.z, localMin.z) };
				bbMax = { std::max(bbMax.x, localMax.x), std::max(bbMax.y, localMax.y), std::max(bbMax.z, localMax.z) };
			}
		}

		if (bbMin == bbMax)
		{
			bbMin = glm::vec3(-1.0f);
			bbMax = glm::vec3(1.0f);
		}

		_sceneDim.min = bbMin;
		_sceneDim.max = bbMax;
		_sceneDim.size = bbMax - bbMin;
		_sceneDim.center = (bbMin + bbMax) * 0.5f;
		_sceneDim.radius = glm::length(bbMax - bbMin) * 0.5f;
	}

	void GLTFLoader::computeCamera()
	{
		for (auto& camera : _sceneCameras)
		{
			if (camera.eye == camera.center)
			{
				camera.eye = camera.world * glm::vec4(camera.eye, 1.0f);
				float distance = glm::length(_sceneDim.center - camera.eye);
				glm::mat3 rot(camera.world);
				camera.center = { 0.0f, 0.0f, -distance };
				camera.center = camera.eye + (rot * camera.center);
				camera.up = { 0.0f, 1.0f, 0.0f };
			}
		}
	}

	void GLTFLoader::importMaterials(const tinygltf::Model& model)
	{
		_sceneMaterials.reserve(model.materials.size());

		for (const auto& mat : model.materials)
		{
			GLTFMaterial material;
			material.name						= mat.name.empty() ? std::to_string(_sceneMaterials.size()) : mat.name;
			material.alphaCutoff				= static_cast<float>(mat.alphaCutoff);
			material.alphaMode					= mat.alphaMode == "MASK" ? 1 : (mat.alphaMode == "BLEND" ? 2 : 0);
			material.doubleSided				= mat.doubleSided ? 1 : 0;
			material.emissiveFactor				= glm::vec3(mat.emissiveFactor[0], mat.emissiveFactor[1], mat.emissiveFactor[2]);
			material.emissiveTexture			= mat.emissiveTexture.index;
			material.normalTexture				= mat.normalTexture.index;
			material.normalTextureScale			= static_cast<float>(mat.normalTexture.scale);
			material.occlusionTexture			= mat.occlusionTexture.index;
			material.occlusionTextureStrength	= static_cast<float>(mat.occlusionTexture.strength);
			
			auto& pbr = mat.pbrMetallicRoughness;
			material.baseColorFactor			= glm::vec4(pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2], pbr.baseColorFactor[3]);
			material.baseColorTexture			= pbr.baseColorTexture.index;
			material.metallicFactor				= static_cast<float>(pbr.metallicFactor);
			material.metallicRoughnessTexture	= pbr.metallicRoughnessTexture.index;
			material.roughnessFactor			= static_cast<float>(pbr.roughnessFactor);
			_sceneMaterials.emplace_back(material);
		}
	}

	void GLTFLoader::releaseSourceData()
	{
		_positions.clear();
		_normals.clear();
		_tangents.clear();
		_colors.clear();
		_texCoords.clear();
		_indices.clear();
		_images.clear();
	}
	
};