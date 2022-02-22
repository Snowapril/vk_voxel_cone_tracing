#version 450

#include "light.glsl"
#include "brdf.glsl"
#include "shadow.glsl"

layout ( constant_id = 0 ) const int MAX_DIRECTIONAL_LIGHT_NUM 	= 8;
layout ( constant_id = 1 ) const int CLIP_LEVEL_COUNT 			= 6;
layout ( constant_id = 2 ) const int VOXEL_FACE_COUNT 			= 6;
layout ( constant_id = 3 ) const int BORDER_WIDTH 				= 1;

layout (location = 0) in VS_OUT {
	vec2 texCoord;
} fs_in;
layout (location = 0) out vec4 diffuseContribution;
layout (location = 1) out vec4 specularContribution;

layout ( set = 0, binding = 0 ) uniform CamMatrix
{ 
 	mat4 uViewProj;		//  64
 	mat4 uViewProjInv;	// 128
 	vec3 uEyePos;		// 140
 	int padding;		// 144
};

// GBuffer binding
layout ( set = 1, binding = 0 ) uniform sampler2D uDiffuseTexture;
layout ( set = 1, binding = 1 ) uniform sampler2D uNormalTexture;
layout ( set = 1, binding = 2 ) uniform sampler2D uSpecularTexture;
layout ( set = 1, binding = 3 ) uniform sampler2D uEmissionTexture;
layout ( set = 1, binding = 4 ) uniform sampler2D uTangentTexture;
layout ( set = 1, binding = 5 ) uniform sampler2D uDepthTexture;

// Voxel clipmap & desc binding
layout ( set = 2, binding = 0 ) uniform sampler3D uVoxelRadiance;

// Shadow map & Light desc binding
layout ( set = 3, binding = 0 ) uniform sampler2D uShadowMaps;
layout ( set = 3, binding = 1 ) uniform DirectionalLight 
{
	DirectionalLightDesc uDirectionalLight;
};
layout ( set = 3, binding = 2 ) uniform DirectionalLightShadow {
	DirectionalLightShadowDesc uDirectionalLightShadow;
};

#define DEBUG_DIFFUSE_ONLY   				0
#define DEBUG_SPECULAR_ONLY  				1
#define DEBUG_NORMAL_ONLY 					2
#define DEBUG_MIN_LEVEL_ONLY				3
#define DEBUG_DIRECT_CONTRIBUTION_ONLY 		4
#define DEBUG_INDIRECT_DIFFUSE_ONLY 		5
#define DEBUG_INDIRECT_SPECULAR_ONLY 		6
#define DEBUG_AMBIENT_OCCLUSION_ONLY 		7
#define DEBUG_GI_OUTPUT 					8
layout ( push_constant ) uniform PushConstants
{
	vec3  uVolumeCenter;			 // 12
	uint  uRenderingMode;			 // 16
	float uVoxelSize;				 // 20
	float uVolumeDimension;			 // 24
	float uTraceStartOffset; 		 // 28
	float uIndirectDiffuseIntensity; // 32
	float uAmbientOcclusionFactor; 	 // 36
	float uMinTraceStepFactor; 		 // 40
	float uIndirectSpecularIntensity;// 44
	float uOcclusionDecay; 			 // 48
	int   uEnable32Cones;			 // 52
};

const float MIN_TRACE_STEP_FACTOR = 0.2;
const float MAX_TRACE_DISTANCE    = 30.0;
const float MIN_SPECULAR_APERTURE = 0.05;

// Get Fixed voxel cone directions from 
// https://www.gamasutra.com/view/news/286023/Graphics_Deep_Dive_Cascaded_voxel_cone_tracing_in_The_Tomorrow_Children.php

// 32 Cones for higher quality (16 on average per hemisphere)
const int 	DIFFUSE_CONE_COUNT_32	  = 32;
const float DIFFUSE_CONE_APERTURE_32  = 0.628319;
const vec3 DIFFUSE_CONE_DIRECTIONS_32[32] = {
    vec3( 0.898904,   0.435512,   0.0479745),
    vec3( 0.898904,  -0.435512,  -0.0479745),
    vec3( 0.898904,   0.0479745, -0.435512 ),
    vec3( 0.898904,  -0.0479745,  0.435512 ),
    vec3(-0.898904,   0.435512,  -0.0479745),
    vec3(-0.898904,  -0.435512,   0.0479745),
    vec3(-0.898904,   0.0479745,  0.435512 ),
    vec3(-0.898904,  -0.0479745, -0.435512 ),
    vec3( 0.0479745,  0.898904,   0.435512 ),
    vec3(-0.0479745,  0.898904,  -0.435512 ),
    vec3(-0.435512,   0.898904,   0.0479745),
    vec3( 0.435512,   0.898904,  -0.0479745),
    vec3(-0.0479745, -0.898904,   0.435512 ),
    vec3( 0.0479745, -0.898904,  -0.435512 ),
    vec3( 0.435512,  -0.898904,   0.0479745),
    vec3(-0.435512,  -0.898904,  -0.0479745),
    vec3( 0.435512,   0.0479745,  0.898904 ),
    vec3(-0.435512,  -0.0479745,  0.898904 ),
    vec3( 0.0479745, -0.435512,   0.898904 ),
    vec3(-0.0479745,  0.435512,   0.898904 ),
    vec3( 0.435512,  -0.0479745, -0.898904 ),
    vec3(-0.435512,   0.0479745, -0.898904 ),
    vec3( 0.0479745,  0.435512,  -0.898904 ),
    vec3(-0.0479745, -0.435512,  -0.898904 ),
    vec3( 0.57735,    0.57735,    0.57735  ),
    vec3( 0.57735,    0.57735,   -0.57735  ),
    vec3( 0.57735,   -0.57735,    0.57735  ),
    vec3( 0.57735,   -0.57735,   -0.57735  ),
    vec3(-0.57735,    0.57735,    0.57735  ),
    vec3(-0.57735,    0.57735,   -0.57735  ),
    vec3(-0.57735,   -0.57735,    0.57735  ),
    vec3(-0.57735,   -0.57735,   -0.57735  )
};

const int 	DIFFUSE_CONE_COUNT_16 		= 16;
const float DIFFUSE_CONE_APERTURE_16 	= 0.872665;
const vec3 DIFFUSE_CONE_DIRECTIONS_16[16] = {
    vec3( 0.57735,   0.57735,   0.57735  ),
    vec3( 0.57735,  -0.57735,  -0.57735  ),
    vec3(-0.57735,   0.57735,  -0.57735  ),
    vec3(-0.57735,  -0.57735,   0.57735  ),
    vec3(-0.903007, -0.182696, -0.388844 ),
    vec3(-0.903007,  0.182696,  0.388844 ),
    vec3( 0.903007, -0.182696,  0.388844 ),
    vec3( 0.903007,  0.182696, -0.388844 ),
    vec3(-0.388844, -0.903007, -0.182696 ),
    vec3( 0.388844, -0.903007,  0.182696 ),
    vec3( 0.388844,  0.903007, -0.182696 ),
    vec3(-0.388844,  0.903007,  0.182696 ),
    vec3(-0.182696, -0.388844, -0.903007 ),
    vec3( 0.182696,  0.388844, -0.903007 ),
    vec3(-0.182696,  0.388844,  0.903007 ),
    vec3( 0.182696, -0.388844,  0.903007 )
};

vec3  worldPosFromDepth	(float depth);
vec4  traceCone			(vec3 startPos, vec3 direction, float aperture, 
						 float maxDistance, float startLevel, float stepFactor);
float calcMinLevel		(vec3 worldPos);
vec4  minLevelToColor	(float minLevel);

void main()
{
	float depth = texture(uDepthTexture, fs_in.texCoord).r;
	if (depth == 1.0)
	{
		discard;
	}

	vec3 worldPos = worldPosFromDepth(depth);
	vec3 view = normalize(uEyePos - worldPos);

	// GBuffer contents
	vec4 diffuse 				= texture(uDiffuseTexture, fs_in.texCoord);
	vec3 diffuseColor 			= diffuse.rgb;
	float perceptualRoughness 	= diffuse.a;
	vec3 normal 				= normalize(texture(uNormalTexture, fs_in.texCoord).rgb * 2.0 - 1.0);
	vec4 specular 				= texture(uSpecularTexture, fs_in.texCoord);
	vec3 specularColor 			= specular.rgb;
	float metallic 				= specular.a;
	vec3 emission 				= texture(uEmissionTexture, fs_in.texCoord).rgb;
	bool hasEmission 			= any(greaterThan(emission, vec3(0.0)));

	// Calculate Indirect contribution
	float minLevel = calcMinLevel(worldPos);
	float voxelSize = uVoxelSize * exp2(minLevel);
	vec3 startPos = worldPos + normal * voxelSize * uTraceStartOffset; 
	
	vec4 indirectContribution = vec4(0.0, 0.0, 0.0, 1.0);

	float validConeCount = 0.0;
	if (uEnable32Cones == 0)
	{
		for (int i = 0; i < DIFFUSE_CONE_COUNT_16; ++i)
		{
			float cosTheta = dot(normal, DIFFUSE_CONE_DIRECTIONS_16[i]);
			if (cosTheta < 0.0) 
				continue;
	
			indirectContribution += traceCone(startPos, DIFFUSE_CONE_DIRECTIONS_16[i], DIFFUSE_CONE_APERTURE_16,
										  	  MAX_TRACE_DISTANCE, minLevel, max(MIN_TRACE_STEP_FACTOR, uMinTraceStepFactor)) * cosTheta; // / 3.141592;
			//validConeCount += cosTheta;
		}
		indirectContribution /= DIFFUSE_CONE_COUNT_16;
	}
	else
	{
		for (int i = 0; i < DIFFUSE_CONE_COUNT_32; ++i)
		{
			float cosTheta = dot(normal, DIFFUSE_CONE_DIRECTIONS_32[i]);
			if (cosTheta < 0.0) 
				continue;
	
			indirectContribution += traceCone(startPos, DIFFUSE_CONE_DIRECTIONS_32[i], DIFFUSE_CONE_APERTURE_32,
										  	  MAX_TRACE_DISTANCE, minLevel, max(MIN_TRACE_STEP_FACTOR, uMinTraceStepFactor)) * cosTheta; // / 3.141592;
			// validConeCount += cosTheta;
		}
		indirectContribution /= DIFFUSE_CONE_COUNT_32;
	}
	// indirectContribution /= validConeCount;
	indirectContribution.a *= uAmbientOcclusionFactor;
	indirectContribution.rgb *= diffuseColor * uIndirectDiffuseIntensity;

	// Specular cone
	vec3 indirectSpecularContribution = vec3(0.0);
	float roughness = sqrt(2.0 / (metallic + 2.0));
	if (any(greaterThan(specularColor, vec3(1e-6))) && metallic > 1e-6)
	{
		vec3 specularConeDirection = reflect(-view, normal);
		indirectSpecularContribution += traceCone(
			startPos, specularConeDirection, 
			max(perceptualRoughness, MIN_SPECULAR_APERTURE), 
			MAX_TRACE_DISTANCE, minLevel, uVoxelSize
		).rgb * specularColor * uIndirectSpecularIntensity;
	}

	vec3 directContribution = vec3(0.0);
	if (hasEmission)
	{
		directContribution += emission;
	}
	else
	{
		// calculate Microfacet BRDF model
		// Roughness is authored as perceptual roughness; as is convention
		// convert to material roughness by squaring the perceptual roughness [2].
		float alphaRoughness = perceptualRoughness * perceptualRoughness;
		
		// For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
		// For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflectance to 0%;
		float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
		vec3 specularEnvironmentR0 = specularColor.rgb;
		vec3 specularEnvironmentR90 = vec3(clamp(reflectance * 50.0, 0.0, 1.0));
	
		vec3 light = normalize(-uDirectionalLight.direction);
		vec3 h = normalize(light + view);
		vec3 reflection = -normalize(reflect(view, normal));
		reflection.y *= -1.0f;

		float NdotL = clamp(dot(normal, light),		0.001, 1.0);
		float NdotV = clamp(abs(dot(normal, view)), 0.001, 1.0);
		float NdotH = clamp(dot(normal, h),			  0.0, 1.0);
		float LdotH = clamp(dot(light, h),			  0.0, 1.0);
		float VdotH = clamp(dot(view, h),			  0.0, 1.0);

		PBRInfo pbr = PBRInfo(NdotL, NdotV, NdotH, LdotH, VdotH, perceptualRoughness,
								  metallic, specularEnvironmentR0, specularEnvironmentR90,
							  alphaRoughness, diffuseColor, specularColor);

		float visibility = calcVisibility(uShadowMaps, uDirectionalLightShadow, worldPos);
		directContribution += microfacetBRDF(pbr) * visibility;
	}

	diffuseContribution = vec4(0.0, 0.0, 0.0, 1.0);
	specularContribution = vec4(0.0, 0.0, 0.0, 1.0);
	
	switch(uRenderingMode)
	{
	case DEBUG_DIFFUSE_ONLY:
		diffuseContribution = vec4(diffuseColor, 1.0);
		break;
	case DEBUG_SPECULAR_ONLY:
		diffuseContribution = vec4(specularColor, 1.0);
		break;
	case DEBUG_NORMAL_ONLY:
		diffuseContribution = vec4(normal * 0.5 + 0.5, 1.0);
		break;
	case DEBUG_MIN_LEVEL_ONLY:
		diffuseContribution = minLevelToColor(minLevel);
		break;
	case DEBUG_DIRECT_CONTRIBUTION_ONLY:
		directContribution *= indirectContribution.a;
		diffuseContribution = vec4(directContribution, 1.0);
		break;
	case DEBUG_INDIRECT_DIFFUSE_ONLY:
		directContribution *= indirectContribution.a;
		diffuseContribution.rgb += directContribution;
		diffuseContribution.rgb += indirectContribution.rgb;
		break;
	case DEBUG_INDIRECT_SPECULAR_ONLY:
		specularContribution.rgb += indirectSpecularContribution;
		break;
	case DEBUG_AMBIENT_OCCLUSION_ONLY:
		diffuseContribution = vec4(vec3(indirectContribution.a), 1.0);
		break;
	case DEBUG_GI_OUTPUT:
		directContribution *= indirectContribution.a;
		diffuseContribution.rgb += directContribution;
		diffuseContribution.rgb += indirectContribution.rgb;
		specularContribution.rgb += indirectSpecularContribution;
		break;
	}
}

ivec3 calculateVoxelFaceIndex(vec3 normal)
{
	return ivec3(
		normal.x > 0.0 ? 0 : 1,
		normal.y > 0.0 ? 2 : 3,
		normal.z > 0.0 ? 4 : 5
	);
}

vec3 worldPosFromDepth(float depth)
{
    vec4 pos = vec4(fs_in.texCoord, depth, 1.0);
    pos.xy = pos.xy * 2.0 - 1.0;
    pos = uViewProjInv * pos;
    return pos.xyz / pos.w;
}

vec4 sampleClipmap(sampler3D clipmap, vec3 worldPos, int clipmapLevel, vec3 faceOffset, vec3 weight)
{
	float voxelSize = uVoxelSize * exp2(clipmapLevel);
	float extent 	=  voxelSize * uVolumeDimension;

	vec3 samplePos = (fract(worldPos / extent) * uVolumeDimension + vec3(BORDER_WIDTH)) / (float(uVolumeDimension) + 2.0 * BORDER_WIDTH);

	samplePos.y += clipmapLevel;
	samplePos.y /= CLIP_LEVEL_COUNT;
	samplePos.x /= VOXEL_FACE_COUNT;

	return texture(clipmap, samplePos + vec3(faceOffset.x, 0.0, 0.0)) * weight.x +
		   texture(clipmap, samplePos + vec3(faceOffset.y, 0.0, 0.0)) * weight.y +
		   texture(clipmap, samplePos + vec3(faceOffset.z, 0.0, 0.0)) * weight.z;
}

vec4 sampleClipmapLinear(sampler3D clipmap, vec3 worldPos, float curLevel, ivec3 faceIndex, vec3 weight)
{
	int lowerLevel = int(floor(curLevel));
	int upperLevel = int( ceil(curLevel));

	vec3 faceOffset  = vec3(faceIndex) / VOXEL_FACE_COUNT;
	vec4 lowerSample = sampleClipmap(clipmap, worldPos, lowerLevel, faceOffset, weight);
	vec4 upperSample = sampleClipmap(clipmap, worldPos, upperLevel, faceOffset, weight);

	return mix(lowerSample, upperSample, fract(curLevel));
}

vec4 traceCone(vec3 startPos, vec3 direction, float aperture, float maxDistance, float startLevel, float stepFactor)
{
	vec4 result = vec4(0.0);
	float coneCoefficient = 2.0 * tan(aperture * 0.5);

	float curLevel = startLevel;
	float voxelSize = uVoxelSize * exp2(curLevel);

	startPos += direction * voxelSize * uTraceStartOffset * 0.5;

	float step 		 = 0.0;
	float diameter 	 = max(step * coneCoefficient, uVoxelSize);
	float occlusion  = 0.0;

	ivec3 faceIndex  = calculateVoxelFaceIndex(direction);
	vec3  weight 	 = direction * direction;

	float curSegmentLength 	= voxelSize;
	float minRadius 		= uVoxelSize * uVolumeDimension * 0.5;

	while((step < maxDistance) && (occlusion < 1.0))
	{
		vec3  position 				= startPos + direction * step;
		float distanceToVoxelCenter = length(uVolumeCenter - position);
		float minLevel 				= ceil(log2(distanceToVoxelCenter / minRadius));

		curLevel = log2(diameter / uVoxelSize);
		curLevel = min(max(max(startLevel, curLevel), minLevel), CLIP_LEVEL_COUNT - 1);

		vec4 clipmapSample = sampleClipmapLinear(uVoxelRadiance, position, curLevel, faceIndex, weight);
		vec3 radiance = clipmapSample.rgb;
		float opacity = clipmapSample.a;

		voxelSize = uVoxelSize * exp2(curLevel);

		float correction = curSegmentLength / voxelSize;
		radiance = radiance * correction;
		opacity  = clamp(1.0 - pow(1.0 - opacity, correction), 0.0, 1.0);

		vec4 src = vec4(radiance.rgb, opacity);
		// Alpha blending
		result 	  += clamp(1.0 - result.a, 0.0, 1.0) * src;
		occlusion += (1.0 - occlusion) * opacity / (1.0 + (step + voxelSize) * uOcclusionDecay);

		float prevStep = step;
		step += max(diameter, uVoxelSize) * stepFactor;
		curSegmentLength = (step - prevStep);
		diameter = step * coneCoefficient;
	}

	return vec4(result.rgb, 1.0 - occlusion);
}

float calcMinLevel(vec3 worldPos)
{
	float distanceToVoxelCenter = length(uVolumeCenter - worldPos);
	float minRadius = uVoxelSize * uVolumeDimension * 0.5;
	float minLevel = max(log2(distanceToVoxelCenter / minRadius), 0.0);

	float radius = minRadius * exp2(ceil(minLevel));
	float f = distanceToVoxelCenter / radius;

	const float transitionStart = 0.5;
	float c = 1.0 / (1.0 - transitionStart);

	if (f > transitionStart)
	{
		return ceil(minLevel) + (f - transitionStart) * c;
	}
	else 
	{
		return ceil(minLevel);
	}
}

vec4 minLevelToColor(float minLevel)
{
   vec4 colors[] = {
        vec4(1.0, 0.0, 0.0, 1.0),
        vec4(0.0, 1.0, 0.0, 1.0),
        vec4(0.0, 0.0, 1.0, 1.0),
        vec4(1.0, 1.0, 0.0, 1.0),
        vec4(0.0, 1.0, 1.0, 1.0),
        vec4(1.0, 0.0, 1.0, 1.0),
        vec4(1.0, 1.0, 1.0, 1.0)
    };
	
    int lowerLevel = int(floor(minLevel));
	vec4 minLevelColor = vec4(mix(colors[lowerLevel], colors[lowerLevel + 1], fract(minLevel)));
	return minLevelColor * 0.5;
}