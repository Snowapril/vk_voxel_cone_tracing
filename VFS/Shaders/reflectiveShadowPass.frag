#version 450

#include "light.glsl"
#include "gltf.glsl"

layout( constant_id = 0 ) const uint MAX_TEXTURE_NUM = 69;

layout (location = 0) in VS_OUT {
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
	vec2 texCoord;
} fs_in;

layout (location = 0) out vec4 rsmPosition;
layout (location = 1) out vec4 rsmNormal;
layout (location = 2) out vec4 rsmFlux;

layout ( std430, set = 1, binding = 1) readonly buffer MaterialBuffer
{
	GltfShadeMaterial uMaterials[];
};

layout ( set = 1, binding = 2 ) uniform sampler2D uTextures[MAX_TEXTURE_NUM];

layout ( set = 2, binding = 1 ) uniform DirectionalLight {
	DirectionalLightDesc uDirectionalLight;
};

layout ( push_constant ) uniform PushConstants
{
	uint uInstanceIndex;
	uint uMaterialIndex;
};

vec3 getNormal(int normalTexture);

void main()
{
	GltfShadeMaterial material = uMaterials[uMaterialIndex];

	if (material.occlusionTexture > -1 && texture(uTextures[material.occlusionTexture], fs_in.texCoord).r < 0.1)
		discard;

	rsmPosition = vec4(fs_in.position, 1.0);
	rsmNormal = vec4(getNormal(material.normalTexture) * 0.5 + 0.5, 1.0);

	vec4 color = material.pbrBaseColorFactor;
	if (material.pbrBaseColorTexture > -1)
	{
		color *= texture(uTextures[material.pbrBaseColorTexture], fs_in.texCoord);
	}

	rsmFlux = vec4(color.rgb * color.a * uDirectionalLight.color * uDirectionalLight.intensity, 1.0);
}

vec3 getNormal(int normalTexture)
{
	if (normalTexture > -1)
	{
		vec3 normalSample = texture(uTextures[normalTexture], fs_in.texCoord).rgb;
		normalSample = 2.0 * normalSample - 1.0;
		
		return normalize(
			normalSample.x * normalize(fs_in.tangent) + 
			normalSample.y * normalize(fs_in.bitangent) + 
			normalSample.z * normalize(fs_in.normal)
		);
	}
	else
	{
		return normalize(fs_in.normal);
	}
}