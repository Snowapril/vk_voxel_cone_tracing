#version 450

#include "gltf.glsl"
#include "light.glsl"
#include "shadow.glsl"

layout( constant_id = 0 ) const uint MAX_TEXTURE_NUM = 69;

layout( location = 0 ) in GS_OUT {
    vec3 position;
    vec3 normal;
    vec2 texCoord;
} fs_in;

layout ( std140, set = 0, binding = 0 ) buffer CounterStorageBuffer
{ 
	uint uCounterValue; 
};
layout ( std430, set = 0, binding = 1 ) writeonly buffer FragmentList
{ 
	uvec2 uFragmentList[]; 
};

layout ( std430, set = 1, binding = 1) readonly buffer MaterialBuffer
{
	GltfShadeMaterial uMaterials[];
};
layout ( set = 1, binding = 2 ) uniform sampler2D uTextures[MAX_TEXTURE_NUM];

layout ( set = 2, binding = 0 ) uniform sampler2D uShadowMaps;
layout ( set = 2, binding = 1 ) uniform DirectionalLight {
	DirectionalLightDesc uDirectionalLight;
};
layout ( set = 2, binding = 2 ) uniform DirectionalLightShadow {
	DirectionalLightShadowDesc uDirectionalLightShadow;
};

layout ( push_constant ) uniform PushConstants
{
	uint uVoxelResolution;
	uint uIsCountMode;
	uint uInstanceIndex;
	uint uMaterialIndex;
};

uint convVec4ToRGBA8( vec4 val);

void main()
{
	GltfShadeMaterial material = uMaterials[uMaterialIndex];

	if (material.occlusionTexture > -1 && texture(uTextures[material.occlusionTexture], fs_in.texCoord).r < 0.1)
		discard;

	uint cur = atomicAdd(uCounterValue, 1u);
	if (uIsCountMode == 0)
	{
		vec4 radiance = vec4(1.0);

		if (any(greaterThan(material.emissiveFactor, vec3(0.0))))
		{
			vec4 emission = vec4(material.emissiveFactor, 1.0);
			if (material.emissiveTexture > -1)
			{
				float lod = log2(float(textureSize(uTextures[material.emissiveTexture], 0).x) / uVoxelResolution);
				emission.rgb += textureLod(uTextures[material.emissiveTexture], fs_in.texCoord, lod).rgb;
			}
			radiance.rgb = clamp(emission.rgb, 0.0, 1.0);
		}
		else
		{
			vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
			if (material.pbrBaseColorTexture > -1)
			{
				float lod = log2(float(textureSize(uTextures[material.pbrBaseColorTexture], 0).x) / uVoxelResolution);
				color = textureLod(uTextures[material.pbrBaseColorTexture], fs_in.texCoord, lod) * material.pbrBaseColorFactor;
			}

			vec3 normal = normalize(fs_in.normal);
			vec3 light = normalize(-uDirectionalLight.direction);

			vec3 lightContribution = vec3(0.0);

			// Calculate light contribution here
			float NdotL = clamp(dot(normal, light), 0.001, 1.0);
			float visibility = calcVisibility(uShadowMaps, uDirectionalLightShadow, fs_in.position);
			lightContribution += NdotL * visibility * uDirectionalLight.color * uDirectionalLight.intensity;
	
			if (all(equal(lightContribution, vec3(0.0))))
				discard;

			radiance = vec4(clamp(lightContribution * color.rgb * color.a, 0.0, 1.0), 1.0);
		}

		uvec3 voxelPos = clamp(uvec3(fs_in.position * uVoxelResolution), 
							   uvec3(0u), 
							   uvec3(uVoxelResolution));

		uFragmentList[cur].x = voxelPos.x | (voxelPos.y << 12u) | ((voxelPos.z & 0xffu) << 24u);
		uFragmentList[cur].y = (voxelPos.z >> 8u) << 28u | (convVec4ToRGBA8(radiance * 255.0) & 0xfffffffu);
		// TODO(snowapril) : need somewhere to store opacity
	}
}

uint convVec4ToRGBA8( vec4 val) {
	return (uint(val.w) & 0x000000FF ) << 24U | 
	(uint(val.z) & 0x000000FF ) << 16U | 
	(uint(val.y) & 0x000000FF ) << 8U | 
	(uint(val.x) & 0x000000FF );
}