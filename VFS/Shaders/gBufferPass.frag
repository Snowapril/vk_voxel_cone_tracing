#version 450

#include "gltf.glsl"

layout( constant_id = 0 ) const uint MAX_TEXTURE_NUM = 69;

layout (location = 0) in VS_OUT {
	vec3 normal;
	vec2 texCoord;
	vec4 tangent;
} fs_in;

layout (location = 0) out vec4 gbufferDiffuse;
layout (location = 1) out vec4 gbufferNormal;
layout (location = 2) out vec4 gbufferSpecular;
layout (location = 3) out vec4 gbufferEmission;
layout (location = 4) out vec4 gbufferTangent;


layout ( std430, set = 1, binding = 1) readonly buffer MaterialBuffer
{
	GltfShadeMaterial uMaterials[];
};

layout ( set = 1, binding = 2 ) uniform sampler2D uTextures[MAX_TEXTURE_NUM];

layout ( push_constant ) uniform PushConstants
{
	uint uInstanceIndex;
	uint uMaterialIndex;
};

#define MIN_ROUGHNESS 0.04

vec4 SRGBtoLinear(vec4 srgbIn, float gamma)
{
	return vec4(pow(srgbIn.xyz, vec3(gamma)), srgbIn.w);
}

vec3 getNormal(int normalTexture)
{
	if (normalTexture > -1)
	{
		vec3 normalSample = texture(uTextures[normalTexture], fs_in.texCoord).rgb;
		normalSample = 2.0 * normalSample - 1.0;
		
		vec3 tangent = fs_in.tangent.xyz;
		vec3 bitangent = cross(fs_in.normal, fs_in.tangent.xyz) * fs_in.tangent.w;

		return normalize(
			normalSample.x * normalize(tangent) 	+ 
			normalSample.y * normalize(bitangent) 	+ 
			normalSample.z * normalize(fs_in.normal)
		);
	}
	else
	{
		return normalize(fs_in.normal);
	}
}

void main()
{
	GltfShadeMaterial material = uMaterials[uMaterialIndex];

	vec3 diffuseColor			= vec3(0.0);
	vec3 specularColor			= vec3(0.0);
	vec4 baseColor				= vec4(0.0, 0.0, 0.0, 1.0);
	vec3 f0						= vec3(0.04);
	float perceptualRoughness;
	float metallic;

	perceptualRoughness = material.pbrRoughnessFactor;
	metallic = material.pbrMetallicFactor;
	// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel
	// This layout intentionally reserves the 'r' channel for (optional) occlusion map data
	if (material.pbrMetallicRoughnessTexture > -1)
	{
		vec4 mrSample = texture(uTextures[material.pbrMetallicRoughnessTexture], fs_in.texCoord);
		perceptualRoughness *= mrSample.g;
		metallic *= mrSample.b;
	}
	else
	{
		perceptualRoughness = clamp(perceptualRoughness, MIN_ROUGHNESS, 1.0);
		metallic = clamp(metallic, 0.0, 1.0);
	}

	baseColor = material.pbrBaseColorFactor;
	if (material.pbrBaseColorTexture > -1)
	{
		baseColor *= texture(uTextures[material.pbrBaseColorTexture], fs_in.texCoord);
	}
	diffuseColor = baseColor.rgb * (vec3(1.0) - f0) * (1.0 - metallic);
	specularColor = mix(f0, baseColor.rgb, metallic);

	if (material.alphaMode > 0 && baseColor.a < material.alphaCutoff)
	{
		discard;
	}
	
	gbufferDiffuse  = vec4(diffuseColor, perceptualRoughness);
	gbufferSpecular = vec4(specularColor, metallic);
	gbufferNormal 	= vec4(getNormal(material.normalTexture) * 0.5 + 0.5, 1.0);
	// gbufferNormal = vec4(normalize(fs_in.normal) * 0.5 + 0.5, 1.0);

	vec4 tangent = vec4(normalize(fs_in.tangent.xyz), fs_in.tangent.w);
	gbufferTangent = vec4(tangent * 0.5 + 0.5);
	
	vec3 emissionColor 			= material.emissiveFactor;
	if (material.emissiveTexture > -1)
	{
		emissionColor *= SRGBtoLinear(texture(uTextures[material.emissiveTexture], fs_in.texCoord), 2.2).rgb;
	}
	gbufferEmission = vec4(emissionColor, 1.0);
}