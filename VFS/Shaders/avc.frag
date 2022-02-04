#version 450

layout (location = 1) in GS_OUT { vec4 color; };
layout (location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(color.xyz, 1.0);
}