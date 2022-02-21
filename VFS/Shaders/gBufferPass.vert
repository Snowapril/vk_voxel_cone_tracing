#version 450

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aTangent;

layout (location = 0) out VS_OUT {
	vec3 normal;
	vec2 texCoord;
	vec4 tangent;
} vs_out;

layout ( set = 0, binding = 0 ) uniform CamMatrix
{ 
 	mat4 uViewProj;
 	mat4 uViewProjInv;
 	vec3 uEyePos;
 	int padding;
};

struct NodeMatrix
{
	mat4 model;
	mat4 itModel;
};

layout ( std430, set = 1, binding = 0) readonly buffer MatrixBuffer
{
	NodeMatrix uNodeMatrices[];
};

layout ( push_constant ) uniform PushConstants
{
	uint uInstanceIndex;
	uint uMaterialIndex;
};

void main()
{
	vs_out.texCoord	 = aTexCoord;
	vs_out.normal	 = (uNodeMatrices[uInstanceIndex].itModel * vec4(aNormal, 0.0)).xyz;
	vs_out.tangent	 = vec4((uNodeMatrices[uInstanceIndex].itModel * vec4(aTangent.xyz, 0.0)).xyz, aTangent.w);

	gl_Position = uViewProj * uNodeMatrices[uInstanceIndex].model * vec4(aPosition, 1.0);
}