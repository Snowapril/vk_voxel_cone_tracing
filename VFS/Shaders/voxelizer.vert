#version 450

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aTangent;

layout( location = 0 ) out VS_OUT {
    vec2 texCoord;
    vec3 normal;
} vs_out;

struct NodeMatrix
{
    mat4 model;
    mat4 itModel;
};

layout ( std430, set = 1, binding = 0) readonly buffer MatrixBuffer
{
    NodeMatrix uNodeMatrices[];
};

layout ( std140, set = 3, binding = 1 ) uniform SceneDimension {  
    vec3 uSceneWorldBBMin;
    vec3 uSceneWorldBBMax;
};

layout ( push_constant ) uniform PushConstants
{
    uint uVoxelResolution;
    uint uIsCountMode;
    uint uInstanceIndex;
    uint uMaterialIndex;
};

void main()
{
    vs_out.texCoord  = aTexCoord;
    vs_out.normal    = (uNodeMatrices[uInstanceIndex].itModel * vec4(aNormal, 0.0)).xyz;

    vec3 extent = uSceneWorldBBMax - uSceneWorldBBMin;
    float extentValue = max(extent.x, max(extent.y, extent.z)) * 0.5;
    vec3 center = (uSceneWorldBBMin + uSceneWorldBBMax) * 0.5;

    vec4 worldPos = uNodeMatrices[uInstanceIndex].model * vec4(aPosition, 1.0);
    gl_Position = vec4((worldPos.xyz - center) / extentValue, 1.0);
}