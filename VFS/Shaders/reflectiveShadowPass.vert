#version 450

#include "light.glsl"

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aTangent;

layout (location = 0) out VS_OUT {
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
	vec2 texCoord;
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

layout ( set = 2, binding = 0 ) uniform DirectionalLightShadow
{
	DirectionalLightShadowDesc uDirLightShadowDesc;
};

layout ( push_constant ) uniform PushConstants
{
	uint uInstanceIndex;
	uint uMaterialIndex;
};

void main()
{
	vec4 modelPos = uNodeMatrices[uInstanceIndex].model * vec4(aPosition, 1.0);
	vs_out.position = modelPos.xyz;
	vs_out.normal	= (uNodeMatrices[uInstanceIndex].itModel * vec4(aNormal, 0.0)).xyz;
	vs_out.tangent	 = (uNodeMatrices[uInstanceIndex].itModel * vec4(aTangent.xyz, 0.0)).xyz;
	vs_out.bitangent = cross(vs_out.normal, vs_out.tangent) * aTangent.w;
	vs_out.texCoord = aTexCoord;

	gl_Position = uDirLightShadowDesc.proj * uDirLightShadowDesc.view * modelPos;
}