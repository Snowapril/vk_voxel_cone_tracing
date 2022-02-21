#version 450

#include "gltf.glsl"
#include "atomic.glsl"
#include "shadow.glsl"
#include "light.glsl"

#define BORDER_WIDTH 1

layout( constant_id = 0 ) const uint MAX_TEXTURE_NUM 	 = 69;
layout( constant_id = 1 ) const uint NUM_POISSON_SAMPLES = 151;

layout( location = 0 ) in GS_OUT {
	vec3 position;
	vec2 texCoord;
	vec3 normal;
} fs_in;

layout ( std430, set = 1, binding = 1) readonly buffer MaterialBuffer
{
	GltfShadeMaterial uMaterials[];
};

layout ( set = 1, binding = 2 ) uniform sampler2D uTextures[MAX_TEXTURE_NUM];

layout ( set = 2, binding = 0, r32ui ) volatile uniform uimage3D uVoxelRadiance;
layout ( set = 2, binding = 1, rgba8 ) uniform readonly image3D uVoxelOpacity;
layout ( std140, set = 2, binding = 2 ) uniform readonly PoissonSamples
{
	vec2 uPoissonSamples[NUM_POISSON_SAMPLES];
};

layout ( std140, set = 3, binding = 2 ) uniform VoxelizationDesc {
	vec3 	uRegionMinCorner;
	uint 	uClipLevel;
	vec3 	uRegionMaxCorner;
	float 	uClipMaxExtent;
	vec3 	uPrevRegionMinCorner;
	float 	uVoxelSize;
	vec3 	uPrevRegionMaxCorner;
	int 	uClipmapResolution;
};

layout ( set = 4, binding = 0 ) uniform sampler2D uRSMPosition;
layout ( set = 4, binding = 1 ) uniform sampler2D uRSMNormal;
layout ( set = 4, binding = 2 ) uniform sampler2D uRSMFlux;
layout ( set = 4, binding = 3 ) uniform sampler2D uRSMShadowMap;
layout ( set = 4, binding = 4 ) uniform DirectionalLight {
	DirectionalLightDesc uDirectionalLight;
};
layout ( set = 4, binding = 5 ) uniform DirectionalLightShadow {
	DirectionalLightShadowDesc uDirectionalLightShadow;
};

layout ( push_constant ) uniform PushConstants
{
	uint uInstanceIndex;
	uint uMaterialIndex;
	// uint uNumDirectionalLight;
};

vec3  worldPosToClipmap			(vec3 pos, float maxExtent);
ivec3 calculateImageCoords		(vec3 worldPos);
ivec3 calculateVoxelFaceIndex	(vec3 normal);
void  voxelAtomicRGBA8Avg 		(ivec3 imageCoord, ivec3 faceIndex, vec4 color, vec3 weight);
void  voxelAtomicRGBA8Avg6Faces	(ivec3 imageCoord, vec4 color);

void main()
{
	GltfShadeMaterial material = uMaterials[uMaterialIndex];
	ivec3 imageCoord = calculateImageCoords(fs_in.position);

	if (material.occlusionTexture > -1 && texture(uTextures[material.occlusionTexture], fs_in.texCoord).r < 0.1)
		discard;

	if (any(greaterThan(material.emissiveFactor, vec3(0.0))))
	{
		vec4 emission = vec4(material.emissiveFactor, 1.0);
		if (material.emissiveTexture > -1)
		{
			float lod = log2(float(textureSize(uTextures[material.emissiveTexture], 0).x) / uClipmapResolution);
			emission.rgb += textureLod(uTextures[material.emissiveTexture], fs_in.texCoord, lod).rgb;
		}
		emission.rgb = clamp(emission.rgb, 0.0, 1.0);
		voxelAtomicRGBA8Avg6Faces(imageCoord, emission);
	}
	else
	{
		// float opacity = imageLoad(uVoxelOpacity, imageCoord).a;
		// if (opacity > 1e-6)
		// {
		// 	vec3 lightSpacePos = (uDirectionalLightShadow.view * vec4(fs_in.position, 1.0)).xyz;
		// 	lightSpacePos.z /= uDirectionalLightShadow.zFar - uDirectionalLightShadow.zNear;
		// 	lightSpacePos.xy = (uDirectionalLightShadow.proj * vec4(lightSpacePos.xy, 0.0, 1.0)).xy;
		// 	lightSpacePos.xy = lightSpacePos.xy * 0.5 + 0.5;
		// 	
		// 	vec3 normal = normalize(fs_in.normal);
		// 	
		// 	float visibility = calcShadowCoefficientPCF16(uRSMShadowMap, vec4(lightSpacePos, 1.0));
		// 	if (visibility > 1e-6)
		// 	{
		// 		vec3 radiance = vec3(0.0);
		// 		for (int i = 0; i < NUM_POISSON_SAMPLES; ++i)
		// 		{
		//			// References on "Reflective Shadow Map" (https://users.soe.ucsc.edu/~pang/160/s13/proposal/mijallen/proposal/media/p203-dachsbacher.pdf)
		// 			vec3 lightSpacePos_p = vec3(lightSpacePos.xy + 0.09 * uPoissonSamples[i], lightSpacePos.z);
		// 			vec3 n_p = textureProj(uRSMNormal, vec4(lightSpacePos_p, 1.0)).xyz * 2.0 - 1.0;
		// 			vec3 x_p = textureProj(uRSMPosition, vec4(lightSpacePos_p, 1.0)).xyz;
		// 			vec3 flux = textureProj(uRSMFlux, vec4(lightSpacePos_p, 1.0)).rgb;
		// 			
		// 			vec3 diff = fs_in.position - x_p;
		// 			float d2 = dot(diff, diff);
		// 			
		// 			vec3 E_p = flux * max(0.0, dot(n_p, diff)) * max(0.0, dot(normal, -diff));
		// 			E_p *= uPoissonSamples[i].x * uPoissonSamples[i].x / abs(d2 * d2);
		// 			
		// 			radiance += E_p;
		// 		}
		// 		
		// 		if (all(equal(radiance, vec3(0.0))))
		// 		{
		// 			discard;
		// 		}
		// 		
		// 		radiance = clamp(radiance * visibility * uDirectionalLight.color * uDirectionalLight.intensity * 0.4, 0.0, 1.0);
		// 		ivec3 faceIndex = calculateVoxelFaceIndex(-normal);
		// 		voxelAtomicRGBA8Avg(imageCoord, faceIndex, vec4(radiance, 1.0), abs(normal));
		// 	}
		// }
		
		vec4 color = material.pbrBaseColorFactor;
		if (material.pbrBaseColorTexture > -1)
		{
			float lod = log2(float(textureSize(uTextures[material.pbrBaseColorTexture], 0).x) / uClipmapResolution);
			color *= textureLod(uTextures[material.pbrBaseColorTexture], fs_in.texCoord, lod);
		}

		vec3 normal = normalize(fs_in.normal);
		vec3 lightDir = normalize(-uDirectionalLight.direction);

		vec3 lightContribution = vec3(0.0);
		// Calculate light contribution here
		float NdotL = clamp(dot(normal, lightDir), 0.001, 1.0);
		float visibility = calcVisibility(uRSMShadowMap, uDirectionalLightShadow, fs_in.position);
		lightContribution += NdotL * visibility * uDirectionalLight.color * uDirectionalLight.intensity;

		if (all(equal(lightContribution, vec3(0.0))))
			discard;

		vec3 radiance = lightContribution * color.rgb * color.a;
		radiance = clamp(radiance, 0.0, 1.0);
		ivec3 faceIndex = calculateVoxelFaceIndex(-normal);
		voxelAtomicRGBA8Avg(imageCoord, faceIndex, vec4(radiance, 1.0), abs(normal));
	}
}

vec3 worldPosToClipmap(vec3 pos, float maxExtent)
{
	return fract(pos / maxExtent);
}

ivec3 calculateImageCoords(vec3 worldPos)
{
	float c = uVoxelSize * 0.25;
	worldPos = clamp(worldPos, float(uRegionMinCorner + c), float(uRegionMaxCorner - c));
	
	vec3 clipCoords = worldPosToClipmap(worldPos, uClipMaxExtent);

	ivec3 imageCoords = ivec3(clipCoords * float(uClipmapResolution)) % uClipmapResolution; // & (uClipmapResolution - 1);
	imageCoords 	 += ivec3(BORDER_WIDTH);
	imageCoords.y 	 += int((uClipmapResolution + 2) * uClipLevel);
	
	return ivec3(imageCoords);
}

ivec3 calculateVoxelFaceIndex(vec3 normal)
{
	return ivec3(
		normal.x > 0.0 ? 0 : 1,
		normal.y > 0.0 ? 2 : 3,
		normal.z > 0.0 ? 4 : 5
	);
}

void imageAtomicRGBA8Avg(ivec3 coords, vec4 value )
{
	value.rgb *= 255.0;
	uint newVal = convVec4ToRGBA8(value);
	uint prevStoredVal = 0; uint curStoredVal;

	// Loop as long as destination value gets changed by other threads
	while ( ( curStoredVal = imageAtomicCompSwap( uVoxelRadiance, coords, prevStoredVal, newVal )) != prevStoredVal) 
	{
		prevStoredVal = curStoredVal;
		vec4 rval = convRGBA8ToVec4(curStoredVal);
		rval.xyz = (rval.xyz * rval.w); 	// Denormalize
		vec4 curValF = rval + value; 		// Add new value
		curValF.xyz /= (curValF.w); 		// Renormalize
		newVal = convVec4ToRGBA8(curValF);
	}
}

void voxelAtomicRGBA8Avg(ivec3 imageCoord, ivec3 faceIndex, vec4 color, vec3 weight)
{
	imageAtomicRGBA8Avg(imageCoord + ivec3((uClipmapResolution + 2) * faceIndex.x, 0, 0), vec4(color.xyz * weight.x, 1.0));
	imageAtomicRGBA8Avg(imageCoord + ivec3((uClipmapResolution + 2) * faceIndex.y, 0, 0), vec4(color.xyz * weight.y, 1.0));
	imageAtomicRGBA8Avg(imageCoord + ivec3((uClipmapResolution + 2) * faceIndex.z, 0, 0), vec4(color.xyz * weight.z, 1.0));
}

void voxelAtomicRGBA8Avg6Faces(ivec3 imageCoord, vec4 color)
{
	for (uint i = 0; i < 6; ++i)
	{
		imageAtomicRGBA8Avg(imageCoord, color);
		imageCoord.x += uClipmapResolution + 2;
	}
}