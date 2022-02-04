#if !defined(SHADOW_GLSL)
#define SHADOW_GLSL

#include "light.glsl"

// These PCF codes are from NVIDIA GPU Gem 1 - Chapter 11 Shadow map antialiasing
// https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing
vec4 offsetLookUp(sampler2D map, vec4 loc, vec2 offset)
{
	vec2 texmapscale = 1.0 / (textureSize(map, 0).xy);
	return textureProj(map, vec4(loc.xy + offset * texmapscale * loc.w, loc.z, loc.w));
}

float calcShadowCoefficientPCF16(sampler2D map, vec4 shadowCoord)
{
	float sum = 0;
	float x, y;
	for (y = -1.5; y <= 1.5; y += 1.0)
	{
		for (x = -1.5; x <= 1.5; x += 1.0)
		{
			sum += offsetLookUp(map, shadowCoord, vec2(x, y)).r;
		}
	}	
	return sum * 0.0625;
}

float calcVisibility(sampler2D map, DirectionalLightShadowDesc lightShadowDesc, vec3 worldPos)
{
	vec3 lightSpacePos = (lightShadowDesc.view * vec4(worldPos, 1.0)).xyz;
	// lightSpacePos.z /= 
	lightSpacePos.xy = (lightShadowDesc.proj * vec4(lightSpacePos.xy, 0.0, 1.0)).xy;
	lightSpacePos.xy = lightSpacePos.xy * 0.5 + 0.5;

	return calcShadowCoefficientPCF16(map, vec4(lightSpacePos, 1.0));
}

#endif