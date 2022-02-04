#version 450

layout (location = 0) in GS_OUT { 
	vec2 texCoord;
	vec4 color;
} fs_in;
layout (location = 0) out vec4 outColor;

void main()
{
	outColor = fs_in.color;
}