#ifndef GLTF_SCENE_IMPL_HPP
#define GLTF_SCENE_IMPL_HPP

#include <Common/GLTFLoader.h>
#include <string>
#include <iostream>
#include <tinygltf/tiny_gltf.h>

namespace vfs 
{

	template <typename Type>
	bool GLTFLoader::GetAttributes(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<Type>& attributes, const char* name)
	{
		auto iter = primitive.attributes.find(name);
		if (iter == primitive.attributes.end())
			return false;

		//! Retrieving the data of the attributes
		const auto& accessor = model.accessors[iter->second];
		const auto& bufferView = model.bufferViews[accessor.bufferView];
		const auto& buffer = model.buffers[bufferView.buffer];
		const Type* bufData = reinterpret_cast<const Type*>(&(buffer.data[accessor.byteOffset + bufferView.byteOffset]));
		const auto& numElements = accessor.count;

		//! Supporting KHR_mesh_quantization
		assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

		if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
		{
			if (bufferView.byteStride == 0)
			{
				attributes.insert(attributes.end(), bufData, bufData + numElements);
			}
			else
			{
				auto bufferByte = reinterpret_cast<const unsigned char*>(bufData);
				for (size_t i = 0; i < numElements; ++i)
				{
					attributes.push_back(*reinterpret_cast<const Type*>(bufferByte));
					bufferByte += bufferView.byteStride;
				}
			}
		}
		else //! The component is smaller than flaot and need to be covnerted (quantized)
		{
			//! vec3 or vec4
			int numComponents = accessor.type;
			//! unsigned byte or unsigned short.
			int strideComponent = accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ? 1 : 2;

			size_t byteStride = bufferView.byteStride > 0 ? bufferView.byteStride : numComponents * strideComponent;
			auto bufferByte = reinterpret_cast<const unsigned char*>(bufData);
			for (size_t i = 0; i < numElements; ++i)
			{
				Type vecValue;

				auto bufferByteData = bufferByte;
				for (int c = 0; c < numComponents; ++c)
				{
					float value = *reinterpret_cast<const float*>(bufferByteData);
					switch (accessor.componentType)
					{
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
						vecValue[c] = value / std::numeric_limits<unsigned short>::max();
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
						vecValue[c] = value / std::numeric_limits<unsigned char>::max();
						break;
					case TINYGLTF_COMPONENT_TYPE_SHORT:
						vecValue[c] = std::max(value / std::numeric_limits<short>::max(), -1.0f);
						break;
					case TINYGLTF_COMPONENT_TYPE_BYTE:
						vecValue[c] = std::max(value / std::numeric_limits<char>::max(), -1.0f);
						break;
					default:
						std::cerr << "Unknown attributes component type : " << accessor.componentType << " is not supported" << std::endl;
						return false;
					}
					bufferByteData += strideComponent;
				}
				bufferByte += byteStride;
				attributes.push_back(vecValue);
			}
		}

		return true;
	}

	template <typename Type>
	inline std::vector<Type> GLTFLoader::GetVector(const tinygltf::Value& value)
	{
		std::vector<Type> retVec{ 0 };
		if (value.IsArray() == false)
			return retVec;

		retVec.resize(value.ArrayLen());
		for (int i = 0; i < static_cast<int>(value.ArrayLen()); ++i)
		{
			retVec[i] = static_cast<Type>(value.Get(i).IsNumber() ? value.Get(i).Get<double>() : value.Get(i).Get<int>());
		}

		return retVec;
	}

	template <>
	inline void GLTFLoader::GetValue<int>(const tinygltf::Value& value, const char* name, int& val)
	{
		if (value.Has(name))
		{
			val = value.Get(name).Get<int>();
		}
	}

	template <>
	inline void GLTFLoader::GetValue<float>(const tinygltf::Value& value, const char* name, float& val)
	{
		if (value.Has(name))
		{
			val = static_cast<float>(value.Get(name).Get<double>());
		}
	}

	template <>
	inline void GLTFLoader::GetValue<glm::vec2>(const tinygltf::Value& value, const char* name, glm::vec2& val)
	{
		if (value.Has(name))
		{
			auto vec = GetVector<float>(value.Get(name));;
			val = glm::vec2(vec[0], vec[1]);
		}
	}

	template <>
	inline void GLTFLoader::GetValue<glm::vec3>(const tinygltf::Value& value, const char* name, glm::vec3& val)
	{
		if (value.Has(name))
		{
			auto vec = GetVector<float>(value.Get(name));;
			val = glm::vec3(vec[0], vec[1], vec[2]);
		}
	}

	template <>
	inline void GLTFLoader::GetValue<glm::vec4>(const tinygltf::Value& value, const char* name, glm::vec4& val)
	{
		if (value.Has(name))
		{
			auto vec = GetVector<float>(value.Get(name));;
			val = glm::vec4(vec[0], vec[1], vec[2], vec[3]);
		}
	}

	inline void GLTFLoader::GetTextureID(const tinygltf::Value& value, const char* name, int& id)
	{
		if (value.Has(name))
		{
			id = value.Get(name).Get("index").Get<int>();
		}
	}

}

#endif