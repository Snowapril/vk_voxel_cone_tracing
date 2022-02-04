#version 450

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aTangent;

struct NodeMatrix
{
	mat4 model;
	mat4 itModel;
};

layout ( std430, set = 1, binding = 0) readonly buffer MatrixBuffer
{
	NodeMatrix uNodeMatrices[];
};

layout ( set = 2, binding = 0 ) uniform LightViewProj
{
	mat4 uLightView;
	mat4 uLightProj;
};

layout ( push_constant ) uniform PushConstants
{
	uint uInstanceIndex;
	uint uMaterialIndex;
};

void main()
{
	gl_Position = uLightProj * uLightView * uNodeMatrices[uInstanceIndex].model * vec4(aPosition, 1.0);
}