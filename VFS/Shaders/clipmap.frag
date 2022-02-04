#version 450

layout (location = 0) in vec2 aTexCoord;
layout (location = 0) out vec4 fragColor;

layout ( set = 1, binding = 0) uniform sampler2D albedoTexture;
layout ( set = 1, binding = 1) uniform sampler2D normalTexture;
layout ( set = 1, binding = 2) uniform sampler2D specularTexture;
layout ( set = 1, binding = 2) uniform sampler2D emissionTexture;

void main()
{
}