#version 450

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aTangent;

layout (location = 0) out vec3 normal;
layout (location = 1) out vec2 texCoord;
layout (location = 2) out vec3 tangent;
layout (location = 3) out vec3 bitangent;

layout ( set = 0, binding = 0 ) uniform CamMatrix
{ 
 	mat4 view;
 	mat4 viewProj;
} uCamMatrix;

struct NodeMatrix
{
	mat4 model;
	mat4 itModel;
};

layout ( std430, set = 1, binding = 0) readonly buffer MatrixBuffer
{
	NodeMatrix nodeMatrices[];
} uModelMatrix;


layout ( push_constant ) uniform PushConstants
{
	uint instanceIndex;
	uint materialIndex;
} uPushConstants;

void main()
{
	texCoord 	= aTexCoord;
	normal 		= (uModelMatrix.nodeMatrices[uPushConstants.instanceIndex].itModel * 
				   vec4(aNormal, 1.0)).xyz;
	tangent 	= (uModelMatrix.nodeMatrices[uPushConstants.instanceIndex].itModel * 
				   vec4(aTangent.xyz, 1.0)).xyz;
	bitangent 	= (uModelMatrix.nodeMatrices[uPushConstants.instanceIndex].itModel * 
				   vec4((cross(normal, tangent) * aTangent.w), 1.0)).xyz;

	gl_Position = uCamMatrix.viewProj * uModelMatrix.nodeMatrices[uPushConstants.instanceIndex].model * vec4(aPosition, 1.0);
}