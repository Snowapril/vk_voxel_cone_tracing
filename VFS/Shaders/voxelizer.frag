#version 450

#include "gltf.glsl"
#include "light.glsl"

layout( constant_id = 0 ) const uint MAX_TEXTURE_NUM = 16;

layout( location = 0 ) in GS_OUT {
    vec3 position;
    vec3 normal;
    vec2 texCoord;
} fs_in;

layout ( std430, set = 1, binding = 1) readonly buffer MaterialBuffer
{
	GltfShadeMaterial uMaterials[];
};

layout ( set = 1, binding = 0 ) uniform sampler2D uTextures[MAX_TEXTURE_NUM];


layout ( std140, set = 2, binding = 0 ) buffer CounterStorageBuffer
{ 
	uint value; 
} uCounter;

layout ( std430, set = 2, binding = 1 ) writeonly buffer FragmentList
{ 
	uvec2 uFragmentList[]; 
};

layout ( set = 3, binding = 0 ) uniform sampler2D uShadowMaps;
layout ( set = 3, binding = 1 ) uniform DirectionalLight {
	DirectionalLightDesc uDirectionalLight;
};

layout ( push_constant ) uniform PushConstants
{
	uint uInstanceIndex;
	uint uMaterialIndex;
	uint uVoxelResolution;
	uint uIsCountMode;
};

void main()
{
	GltfShadeMaterial material = uMaterials[uMaterialIndex];

	if (material.occlusionTexture > -1 && texture(uTextures[material.occlusionTexture], fs_in.texCoord).r < 0.1)
		discard;

	uint cur = atomicAdd(uCounter.value, 1u);
	if (uIsCountMode == 0)
	{
		vec4 radiance = vec4(1.0);

		if ((material.emissiveFactor.x > 0.0) || (material.emissiveFactor.y > 0.0) || (material.emissiveFactor.z > 0.0))
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
			vec3 lightContribution = vec3(0.0);

			// Calculate light contribution here
			float NdotL = max(dot(normal, -uDirectionalLight.direction), 0.0);
			float visibility = 1.0; // TODO(snowapril) : calcShadowCoefficientPCF16(uShadowMaps, fs_in.position);
			lightContribution += NdotL * visibility * uDirectionalLight.color * uDirectionalLight.intensity;
	
			if ((lightContribution.r == 0.0) && (lightContribution.g == 0.0) && (lightContribution.b == 0.0))
				discard;

			radiance = vec4(clamp(lightContribution * color.rgb * color.a, 0.0, 1.0), 1.0);
		}

		uvec3 voxelPos = clamp(uvec3(fs_in.position * uVoxelResolution), 
							   uvec3(0u), 
							   uvec3(uVoxelResolution));

		uint radianceUI = packUnorm4x8(radiance);
		uFragmentList[cur].x = voxelPos.x | (voxelPos.y << 12u) | ((voxelPos.z & 0xffu) << 24u);
		uFragmentList[cur].y = (voxelPos.z >> 8u) << 28u | (radianceUI & 0xfffffffu);
		// TODO(snowapril) : need somewhere to store opacity
	}
}