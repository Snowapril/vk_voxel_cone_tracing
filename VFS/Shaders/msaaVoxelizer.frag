#version 450

#include "gltf.glsl"

#define BORDER_WIDTH 2

layout( constant_id = 0 ) const uint MAX_TEXTURE_NUM = 69;

layout( location = 0 ) in GS_OUT {
	vec3 position;
	vec2 texCoord;
} fs_in;

layout ( std430, set = 1, binding = 1) readonly buffer MaterialBuffer
{
	GltfShadeMaterial uMaterials[];
};
layout ( set = 1, binding = 2 ) uniform sampler2D uTextures[MAX_TEXTURE_NUM];

layout ( set = 2, binding = 0, rgba8) uniform writeonly image3D uVoxelOpacity;

layout ( set = 2, binding = 1 ) buffer CounterStorageBuffer
{ 
	uint value; 
} uCounter;

layout ( std430, set = 2, binding = 2 ) writeonly buffer ImageCoordBuffer {
	ivec3 data[];
} uImageCoordBuffer;

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

layout ( push_constant ) uniform PushConstants
{
	uint uInstanceIndex;
	uint uMaterialIndex;
};

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
	// ivec3 imageCoords = ivec3(float(uClipmapResolution) * (0.5 * worldPos + 0.5));
	imageCoords.z  	+= BORDER_WIDTH;
	imageCoords.y 	+= int((uClipmapResolution + 2) * uClipLevel);
	
	return imageCoords;
}

void main()
{
	if (any(lessThan(fs_in.position, uRegionMinCorner)) || any(greaterThan(fs_in.position, uRegionMaxCorner)))
		discard;

	GltfShadeMaterial material = uMaterials[uMaterialIndex];

	if (material.occlusionTexture > -1 && texture(uTextures[material.occlusionTexture], fs_in.texCoord).r < 0.1)
		discard;

	ivec3 imageCoord = calculateImageCoords(fs_in.position);
	uImageCoordBuffer.data[atomicAdd(uCounter.value, 1u)] = imageCoord;

	for (uint i = 0; i < 6; ++i)
	{
		imageStore(uVoxelOpacity, imageCoord, vec4(1.0));
		imageCoord.x += uClipmapResolution + 2;
	}
}