#version 450

#include "filter.glsl"
#include "tonemapping.glsl"

layout (location = 0) in VS_OUT {
	vec2 texCoord;
} fs_in;
layout (location = 0) out vec4 finalOutputColor;

layout ( set = 0, binding = 0 ) uniform sampler2D uDiffuseContribution;
layout ( set = 0, binding = 1 ) uniform sampler2D uSpecularContribution;

#define USE_BILATERAL_FILTER 0
#define USE_GAUSSIAN_BLUR 	 1

layout ( push_constant ) uniform PushConstants
{
	float uTonemapGamma; 	//  4
	float uTonemapExposure; //  8
	int   uTonemapEnable;	// 12
	int   uFilterMethod; 	// 16
};

void main()
{
	vec4 finalColor = vec4(0.0, 0.0, 0.0, 1.0);
	finalColor = texture(uDiffuseContribution, fs_in.texCoord);

	vec4 specularContribution = vec4(0.0, 0.0, 0.0, 1.0);
	switch (uFilterMethod)
	{
	case USE_BILATERAL_FILTER:
		specularContribution.rgb += bilateralFilter(uSpecularContribution, fs_in.texCoord);
		break;
	case USE_GAUSSIAN_BLUR:
		specularContribution = gaussianBlur(uSpecularContribution, fs_in.texCoord, 0.01);
		break;
	default:
		specularContribution.rgb += bilateralFilter(uSpecularContribution, fs_in.texCoord);
		break;
	}
    finalColor.rgb += specularContribution.rgb;

	if (uTonemapEnable == 1)
	{
		finalOutputColor = tonemap(finalColor, uTonemapGamma, uTonemapExposure);
	}
	else
	{
		finalOutputColor = finalColor;
	}
}	