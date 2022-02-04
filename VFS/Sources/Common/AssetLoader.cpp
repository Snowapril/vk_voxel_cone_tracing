// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <Common/AssetLoader.h>
#include <Common/Utils.h>
#include <iostream>
#include <string>

#include <stb/stb_image.h>

#pragma warning (push)
#pragma warning (disable : 26451)
#pragma warning (disable : 26495)
#pragma warning (disable : 26498)
#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>
#pragma warning (pop)

namespace vfs
{
    bool HasSmoothingGroup(const tinyobj::shape_t& shape)
    {
        for (size_t i = 0; i < shape.mesh.smoothing_group_ids.size(); i++)
        {
            if (shape.mesh.smoothing_group_ids[i] > 0)
            {
                return true;
            }
        }
        return false;
    }

    bool CheckTriangle(const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 v3)
    {
        return (v2.x - v1.x) * (v3.y - v2.y) != (v3.x - v2.x) * (v2.y - v1.y);
    }

    inline glm::vec3 CalculateNormal(const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 v3)
    {
        if (!CheckTriangle(v1, v2, v3))
            return glm::vec3(0.0f);

        glm::vec3 edge1 = v2 - v1;
        glm::vec3 edge2 = v3 - v2;
        return glm::normalize(glm::cross(edge1, edge2));
    }

    void ComputeSmoothingNormals(const tinyobj::attrib_t& attrib, const tinyobj::shape_t& shape, std::map<int, glm::vec3>& smoothVertexNormals)
    {
        smoothVertexNormals.clear();
        const auto& vertices = attrib.vertices;

        for (size_t faceIndex = 0; faceIndex < shape.mesh.indices.size() / 3; faceIndex++)
        {
            tinyobj::index_t idx0 = shape.mesh.indices[3 * faceIndex + 0];
            tinyobj::index_t idx1 = shape.mesh.indices[3 * faceIndex + 1];
            tinyobj::index_t idx2 = shape.mesh.indices[3 * faceIndex + 2];

            glm::vec3 position[3];

            int f0 = idx0.vertex_index;
            int f1 = idx1.vertex_index;
            int f2 = idx2.vertex_index;
            assert(f0 >= 0 && f1 >= 0 && f2 >= 0);

            position[0] = glm::vec3(vertices[3 * f0], vertices[3 * f0 + 1], vertices[3 * f0 + 2]);
            position[1] = glm::vec3(vertices[3 * f1], vertices[3 * f1 + 1], vertices[3 * f1 + 2]);
            position[2] = glm::vec3(vertices[3 * f2], vertices[3 * f2 + 1], vertices[3 * f2 + 2]);

            glm::vec3 normal = CalculateNormal(position[0], position[1], position[2]);

            int faces[3] = { f0, f1, f2 };
            for (size_t i = 0; i < 3; ++i)
            {
                auto iter = smoothVertexNormals.find(faces[i]);
                if (iter == smoothVertexNormals.end())
                {
                    smoothVertexNormals[faces[i]] = normal;
                }
                else
                {
                    iter->second += normal;
                }
            }
        }

        for (auto& p : smoothVertexNormals)
        {
            p.second = glm::normalize(p.second);
        }
    }

    struct PackedVertex
    {
        glm::vec3 position;
        glm::vec2 texCoord;
        glm::vec3 normal;

        PackedVertex(glm::vec3 pos, glm::vec2 uv, glm::vec3 n)
            : position(pos), texCoord(uv), normal(n) {};
    };

    //! lexicographically sorting vector.
    inline bool operator<(const PackedVertex& v1, const PackedVertex& v2)
    {
        if (std::fabs(v1.position.x - v2.position.x) >= 0.001f) return v1.position.x < v2.position.x;
        if (std::fabs(v1.position.y - v2.position.y) >= 0.001f) return v1.position.y < v2.position.y;
        if (std::fabs(v1.position.z - v2.position.z) >= 0.001f) return v1.position.z < v2.position.z;
        if (std::fabs(v1.texCoord.x - v2.texCoord.x) >= 0.1f  ) return v1.texCoord.x < v2.texCoord.x;
        if (std::fabs(v1.texCoord.y - v2.texCoord.y) >= 0.1f  ) return v1.texCoord.y < v2.texCoord.y;
        if (std::fabs(v1.normal.x - v2.normal.x)     >= 0.3f  ) return v1.normal.x < v2.normal.x;
        if (std::fabs(v1.normal.y - v2.normal.y)     >= 0.3f  ) return v1.normal.y < v2.normal.y;
        if (std::fabs(v1.normal.z - v2.normal.z)     >= 0.3f  ) return v1.normal.z < v2.normal.z;
        return false;
    }

	bool AssetLoader::LoadObjFile(const char*				 objPath,
								  OUT std::vector<Mesh>*	 meshes,
								  VertexFormat				 format)
	
	{
        tinyobj::attrib_t                attrib;
        std::vector<tinyobj::shape_t>    shapes;
        std::vector<tinyobj::material_t> materials;
        std::string                      warn;
        std::string                      err;

        char baseDir[128];
        getDirectory(objPath, baseDir, sizeof(baseDir));
        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPath, baseDir))
        {
            std::cerr << "[Common] Failed to load " << objPath << '\n';
            return false;
        }
        if (shapes.empty())
        {
            std::cerr << "[Common] No mesh shapes in " << objPath << '\n';
            return false;
        }

        // reserve for memory
        meshes->resize(materials.size());

        const double invNumFloats = 1.0 / VertexHelper::GetNumFloats(format);

        for (auto& shape : shapes)
        {
            std::map<int, glm::vec3> smoothVertexNormals;
            if (HasSmoothingGroup(shape))
            {
                ComputeSmoothingNormals(attrib, shape, smoothVertexNormals);
            }

            for (size_t faceIndex = 0; faceIndex < shape.mesh.indices.size() / 3; ++faceIndex)
            {
                const int materialID = shape.mesh.material_ids[faceIndex];
                const tinyobj::material_t& material = materials[materialID];
                Mesh& newMesh = meshes->at(materialID);
                newMesh.ambientTexPaths     = material.ambient_texname;
                newMesh.diffuseTexPaths     = material.diffuse_texname;
                newMesh.specularTexPaths    = material.specular_texname;
                newMesh.emissiveTexPaths    = material.emissive_texname;
                newMesh.opacityTexPaths     = material.alpha_texname;
                newMesh.normalTexPaths      = material.normal_texname;
                newMesh.diffuseColor        = { material.diffuse[0],  material.diffuse[1], material.diffuse[2] };
                newMesh.specularColor       = { material.specular[0], material.specular[1], material.specular[2] };
                newMesh.emissiveColor       = { material.emission[0], material.emission[1], material.emission[2] };
                newMesh.shininess           = material.shininess;
                newMesh.opacity             = material.dissolve;

                /*
                idx0 (pos (float3), normal(float3), texcoords(float2))
                |\
                | \
                |  \
                |   \ idx2 (pos (float3), normal(float3), texcoords(float2))
                |   /
                |  /
                | /
                |/
                idx1 (pos (float3), normal(float3), texcoords(float2))
                */
                tinyobj::index_t idx0 = shape.mesh.indices[3 * faceIndex + 0];
                tinyobj::index_t idx1 = shape.mesh.indices[3 * faceIndex + 1];
                tinyobj::index_t idx2 = shape.mesh.indices[3 * faceIndex + 2];

                glm::vec3 position[3] {};
                glm::vec2 texCoord[3] {};
                glm::vec3 normal[3]{};
                glm::vec4 tangent[3]{};

                if (static_cast<bool>(format & VertexFormat::Position3))
                {
                    for (int k = 0; k < 3; k++)
                    {
                        int f0 = idx0.vertex_index;
                        int f1 = idx1.vertex_index;
                        int f2 = idx2.vertex_index;
                        assert(f0 >= 0 && f1 >= 0 && f2 >= 0);

                        position[0][k] = attrib.vertices[3 * f0 + k];
                        position[1][k] = attrib.vertices[3 * f1 + k];
                        position[2][k] = attrib.vertices[3 * f2 + k];
                    }

                    newMesh.positions.emplace_back(position[0]);
                    newMesh.positions.emplace_back(position[1]);
                    newMesh.positions.emplace_back(position[2]);
                }

                if (static_cast<bool>(format & VertexFormat::Normal3))
                {
                    bool invalidNormal = false;
                    if (attrib.normals.size() > 0)
                    {
                        int f0 = idx0.normal_index;
                        int f1 = idx1.normal_index;
                        int f2 = idx2.normal_index;
                        if (f0 < 0 || f1 < 0 || f2 < 0)
                        {
                            invalidNormal = true;
                        }
                        else
                        {
                            for (size_t k = 0; k < 3; k++)
                            {
                                assert(size_t(3 * f0 + k) < attrib.normals.size());
                                assert(size_t(3 * f1 + k) < attrib.normals.size());
                                assert(size_t(3 * f2 + k) < attrib.normals.size());
                                normal[0][k] = attrib.normals[3 * f0 + k];
                                normal[1][k] = attrib.normals[3 * f1 + k];
                                normal[2][k] = attrib.normals[3 * f2 + k];
                            }
                        }
                    }
                    else
                    {
                        invalidNormal = true;
                    }
                    if (invalidNormal)
                    {
                        if (!smoothVertexNormals.empty())
                        {
                            int f0 = idx0.vertex_index;
                            int f1 = idx1.vertex_index;
                            int f2 = idx2.vertex_index;
                            if (f0 >= 0 && f1 >= 0 && f2 >= 0)
                            {
                                normal[0] = smoothVertexNormals[f0];
                                normal[1] = smoothVertexNormals[f1];
                                normal[2] = smoothVertexNormals[f2];
                            }
                        }
                        else
                        {
                            normal[0] = CalculateNormal(position[0], position[1], position[2]);
                            normal[1] = normal[0];
                            normal[2] = normal[0];
                        }
                    }

                    newMesh.normals.emplace_back(normal[0]);
                    newMesh.normals.emplace_back(normal[1]);
                    newMesh.normals.emplace_back(normal[2]);
                }

                if (static_cast<bool>(format & VertexFormat::TexCoord2))
                {
                    if (attrib.texcoords.size() > 0)
                    {
                        int f0 = idx0.texcoord_index;
                        int f1 = idx1.texcoord_index;
                        int f2 = idx2.texcoord_index;

                        if (f0 < 0 || f1 < 0 || f2 < 0)
                        {
                            texCoord[0] = glm::vec2(0.0f, 0.0f);
                            texCoord[1] = glm::vec2(0.0f, 0.0f);
                            texCoord[2] = glm::vec2(0.0f, 0.0f);
                        }
                        else
                        {
                            assert(attrib.texcoords.size() > size_t(2 * f0 + 1));
                            assert(attrib.texcoords.size() > size_t(2 * f1 + 1));
                            assert(attrib.texcoords.size() > size_t(2 * f2 + 1));

                            // Flip Y coord.
                            texCoord[0] = glm::vec2(attrib.texcoords[2 * f0], 1.0f - attrib.texcoords[2 * f0 + 1]);
                            texCoord[1] = glm::vec2(attrib.texcoords[2 * f1], 1.0f - attrib.texcoords[2 * f1 + 1]);
                            texCoord[2] = glm::vec2(attrib.texcoords[2 * f2], 1.0f - attrib.texcoords[2 * f2 + 1]);
                        }
                    }
                    else
                    {
                        texCoord[0] = glm::vec2(0.0f, 0.0f);
                        texCoord[1] = glm::vec2(0.0f, 0.0f);
                        texCoord[2] = glm::vec2(0.0f, 0.0f);
                    }

                    newMesh.texCoords.emplace_back(texCoord[0]);
                    newMesh.texCoords.emplace_back(texCoord[1]);
                    newMesh.texCoords.emplace_back(texCoord[2]);
                }

                // Implementation in "Foundations of Game Engine Development : Volume2 Rendering"
                if (static_cast<bool>(format & VertexFormat::Tangent4) &&
                    static_cast<bool>(format & VertexFormat::Position3) &&
                    static_cast<bool>(format & VertexFormat::TexCoord2))
                {
                    const glm::vec3& pos0 = position[0];
                    const glm::vec3& pos1 = position[1];
                    const glm::vec3& pos2 = position[2];

                    const glm::vec2& uv0 = texCoord[0];
                    const glm::vec2& uv1 = texCoord[1];
                    const glm::vec2& uv2 = texCoord[2];

                    glm::vec3 e1 = pos1 - pos0, e2 = pos2 - pos0;
                    float x1 = uv1.x - uv0.x, x2 = uv2.x - uv0.x;
                    float y1 = uv1.y - uv0.y, y2 = uv2.y - uv0.y;

                    const float r = 1.0f / (x1 * y2 - x2 * y1);
                    glm::vec3 t = (e1 * y2 - e2 * y1) * r;
                    glm::vec3 b = (e2 * x1 - e1 * x2) * r;

                    // In case of degenerated UV coordinates
                    if (x1 == 0 || x2 == 0 || y1 == 0 || y2 == 0)
                    {
                        const glm::vec3& normal0 = normal[0];
                        const glm::vec3& normal1 = normal[1];
                        const glm::vec3& normal2 = normal[2];
                        const auto N = (normal0 + normal1 + normal2) / glm::vec3(3.0f);

                        if (std::abs(N.x) > std::abs(N.y))
                            t = glm::vec3(N.z, 0, -N.x) / std::sqrt(N.x * N.x + N.z * N.z);
                        else
                            t = glm::vec3(0, -N.z, N.y) / std::sqrt(N.y * N.y + N.z * N.z);
                        b = glm::cross(N, t);
                    }
                    
                    for (uint32_t i = 0; i < 3; ++i)
                    {
                        const glm::vec3& norm = normal[i];
                        // gram schmidt orthogonalize
                        glm::vec3 ot = glm::normalize(t - norm * glm::vec3(glm::dot(norm, t)));
                        // calculate handedness
                        float handedness = (glm::dot(glm::cross(t, b), norm) > 0.0f) ? 1.0f : -1.0f;
                        tangent[i] = glm::vec4(ot.x, ot.y, ot.z, handedness);
                    }

                    newMesh.tangents.emplace_back(tangent[0]);
                    newMesh.tangents.emplace_back(tangent[1]);
                    newMesh.tangents.emplace_back(tangent[2]);
                }

                /*for (unsigned int k = 0; k < 3; ++k)
                {
                    PackedVertex vertex(position[k], texCoord[k], normal[k]);

                    auto iter = packedVerticesMap.find(vertex);

                    if (iter == packedVerticesMap.end())
                    {
                        if (static_cast<int>(format & VertexFormat::Position3))
                        {
                            vertices->insert(vertices->end(), { position[k].x, position[k].y, position[k].z });
                            minCorner->x = std::min(position[k].x, minCorner->x);
                            minCorner->y = std::min(position[k].y, minCorner->y);
                            minCorner->z = std::min(position[k].z, minCorner->z);
                            maxCorner->x = std::max(position[k].x, maxCorner->x);
                            maxCorner->y = std::max(position[k].y, maxCorner->y);
                            maxCorner->z = std::max(position[k].z, maxCorner->z);
                        }
                        if (static_cast<int>(format & VertexFormat::Normal3))
                        {
                            vertices->insert(vertices->end(), { normal[k].x, normal[k].y, normal[k].z });
                        }
                        if (static_cast<int>(format & VertexFormat::TexCoord2))
                        {
                            vertices->insert(vertices->end(), { texCoord[k].x, texCoord[k].y });
                        }
                        const uint32_t newIndex = static_cast<uint32_t>(vertices->size() * invNumFloats);
                        indices->push_back(newIndex);
                        packedVerticesMap[vertex] = newIndex;
                    }
                    else
                    {
                        indices->push_back(iter->second);
                    }
                }*/
            }
        }

        return true;
	}

    bool AssetLoader::LoadImageU8(const char* imagePath, 
								  OUT uint8_t** data, 
								  OUT int* width, 
								  OUT int* height, 
								  OUT int* numChannels)
    {
        *data = stbi_load(imagePath, width, height, numChannels, STBI_rgb_alpha);
        if (*data == nullptr || *width == 0 || *height == 0 || *numChannels == 0)
        {
            return false;
        }
        return true;
    }

    void AssetLoader::FreeImage(void* data)
    {
        stbi_image_free(data);
    }
}