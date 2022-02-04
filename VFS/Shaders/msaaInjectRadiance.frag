#version 450

#include "gltf.glsl"
#include "atomic.glsl"
#include "shadow.glsl"
#include "light.glsl"

#define BORDER_WIDTH 2

layout( constant_id = 0 ) const uint MAX_TEXTURE_NUM = 69;

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

layout ( set = 4, binding = 0 ) uniform sampler2D uShadowMaps;

layout ( set = 4, binding = 1 ) uniform  DirectionalLight {
	DirectionalLightDesc uDirectionalLight;
};

// layout ( set = 4, binding = 2 ) uniform DirectionalLightShadowDesc {
// 	DirectionalLightShadowDesc uDirectionalLightShadow;
// };

layout ( push_constant ) uniform PushConstants
{
	uint uInstanceIndex;
	uint uMaterialIndex;
	// uint uNumDirectionalLight;
};

vec3  worldPosToClipmap			(vec3 pos, float maxExtent);
ivec3 calculateImageCoords		(vec3 worldPos);
ivec3 calculateVoxelFaceIndex	(vec3 normal);
void  voxelAtomicRGBA8Avg 		(vec3 worldPos, ivec3 faceIndex, vec4 color, vec3 weight);
void  voxelAtomicRGBA8Avg6Faces	(vec3 worldPos, vec4 color);

void main()
{
	GltfShadeMaterial material = uMaterials[uMaterialIndex];

	if (material.occlusionTexture > -1 && texture(uTextures[material.occlusionTexture], fs_in.texCoord).r < 0.1)
		discard;

	if ((material.emissiveFactor.x > 0.0) || (material.emissiveFactor.y > 0.0) || (material.emissiveFactor.z > 0.0))
	{
		vec4 emission = vec4(material.emissiveFactor, 1.0);
		if (material.emissiveTexture > -1)
		{
			float lod = log2(float(textureSize(uTextures[material.emissiveTexture], 0).x) / uClipmapResolution);
			emission.rgb += textureLod(uTextures[material.emissiveTexture], fs_in.texCoord, lod).rgb;
		}
		emission.rgb = clamp(emission.rgb, 0.0, 1.0);
		voxelAtomicRGBA8Avg6Faces(fs_in.position, emission);
	}
	else
	{
		vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
		if (material.pbrBaseColorTexture > -1)
		{
			float lod = log2(float(textureSize(uTextures[material.pbrBaseColorTexture], 0).x) / uClipmapResolution);
			color = textureLod(uTextures[material.pbrBaseColorTexture], fs_in.texCoord, lod) * material.pbrBaseColorFactor;
		}

		vec3 normal = normalize(fs_in.normal);
		vec3 lightContribution = vec3(0.0);

		// Calculate light contribution here
		float NdotL = max(dot(normal, -uDirectionalLight.direction), 0.0);
		float visibility = 1.0; // TODO(snowapril) : calcShadowCoefficientPCF16(uShadowMaps, fs_in.position);
		lightContribution += NdotL * visibility * uDirectionalLight.color * uDirectionalLight.intensity;

		if (all(equal(lightContribution, vec3(0.0))))
			discard;

		vec3 radiance = lightContribution * color.rgb * color.a; // TODO(snowapril) : different diffuse with reference
		radiance = clamp(radiance, 0.0, 1.0);

		ivec3 faceIndex = calculateVoxelFaceIndex(-normal);
		voxelAtomicRGBA8Avg(fs_in.position, faceIndex, vec4(radiance, 1.0), abs(normal));
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

	ivec3 imageCoords = ivec3(clipCoords * float(uClipmapResolution)) & (uClipmapResolution - 1);
	imageCoords += ivec3(BORDER_WIDTH);
	imageCoords.y += int((uClipmapResolution + BORDER_WIDTH) * uClipLevel);
	
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

void voxelAtomicRGBA8Avg(vec3 worldPos, ivec3 faceIndex, vec4 color, vec3 weight)
{
	ivec3 imageCoord = calculateImageCoords(worldPos);

	imageAtomicRGBA8Avg(imageCoord + ivec3((uClipmapResolution + BORDER_WIDTH) * faceIndex.x, 0, 0), vec4(color.xyz * weight.x, 1.0));
	imageAtomicRGBA8Avg(imageCoord + ivec3((uClipmapResolution + BORDER_WIDTH) * faceIndex.y, 0, 0), vec4(color.xyz * weight.y, 1.0));
	imageAtomicRGBA8Avg(imageCoord + ivec3((uClipmapResolution + BORDER_WIDTH) * faceIndex.z, 0, 0), vec4(color.xyz * weight.z, 1.0));
}

void voxelAtomicRGBA8Avg6Faces(vec3 worldPos, vec4 color)
{
	ivec3 imageCoord = calculateImageCoords(worldPos);
	for (uint i = 0; i < 6; ++i)
	{
		imageAtomicRGBA8Avg(imageCoord, color);
		imageCoord.x += uClipmapResolution + BORDER_WIDTH;
	}
}