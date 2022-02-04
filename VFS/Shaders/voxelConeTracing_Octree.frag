#version 450

#include "light.glsl"
#include "brdf.glsl"
#include "shadow.glsl"

layout ( constant_id = 0 ) const int MAX_DIRECTIONAL_LIGHT_NUM 	= 8;
layout ( constant_id = 1 ) const int CLIP_LEVEL_COUNT 			= 6;
layout ( constant_id = 2 ) const int VOXEL_FACE_COUNT 			= 6;
#define BORDER_WIDTH 1

layout (location = 0) in VS_OUT {
	vec2 texCoord;
} fs_in;
layout (location = 0) out vec4 fragColor;

layout ( set = 0, binding = 0 ) readonly uniform CamMatrix
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
layout ( set = 1, binding = 4 ) uniform sampler2D uDepthTexture;

// Sparse voxel octree & desc binding
layout ( std430, set = 2, binding = 0 ) readonly buffer OctreeStorageBuffer { 
    uvec2 uVoxelOctreeNode[]; 
};
layout ( set = 2, binding = 1 ) readonly uniform VoxelConeTracingDesc {
	vec3  uVolumeCenter;
	float uVoxelSize;
	float uVolumeDimension;
};

// Shadow map & Light desc binding
layout ( set = 3, binding = 0 ) uniform sampler2D uShadowMaps;
layout ( set = 3, binding = 1 ) readonly uniform DirectionalLight {
	DirectionalLightDesc uDirectionalLight;
};
layout ( set = 3, binding = 2 ) readonly uniform DirectionalLightShadow {
	DirectionalLightShadowDesc uDirectionalLightShadow;
};

#define DEBUG_DIFFUSE_ONLY   				0
#define DEBUG_SPECULAR_ONLY  				1
#define DEBUG_ROUGHNESS_ONLY 				2
#define DEBUG_DIRECT_CONTRIBUTION_ONLY 		3
#define DEBUG_INDIRECT_CONTRIBUTION_ONLY 	4
#define DEBUG_AMBIENT_OCCLUSION_ONLY 		5
#define DEBUG_GI_OUTPUT 					6
layout ( push_constant ) uniform PushConstants
{
	uint uRenderingMode;
};

const float MIN_TRACE_STEP_FACTOR = 0.2;
const float MAX_TRACE_DISTANCE    = 30.0;

// Get Fixed voxel cone directions from 
// https://www.gamasutra.com/view/news/286023/Graphics_Deep_Dive_Cascaded_voxel_cone_tracing_in_The_Tomorrow_Children.php

// #define FIXED_32_CONES
#if defined(FIXED_32_CONES)
// 32 Cones for higher quality (16 on average per hemisphere)
const int 	DIFFUSE_CONE_COUNT	  = 32;
const float DIFFUSE_CONE_APERTURE = 0.628319;

const vec3 DIFFUSE_CONE_DIRECTIONS[32] = {
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
#else
const int 	DIFFUSE_CONE_COUNT 		= 16;
const float DIFFUSE_CONE_APERTURE 	= 0.872665;

const vec3 DIFFUSE_CONE_DIRECTIONS[16] = {
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
#endif

vec3  worldPosFromDepth	(float depth);
vec4  traceCone			(vec3 startPos, vec3 direction, float aperture, 
						 float maxDistance, float startLevel);
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
	vec4 diffuse = texture(uDiffuseTexture, fs_in.texCoord);
	vec3 diffuseColor = diffuse.rgb;
	float perceptualRoughness = diffuse.a;
	vec3 normal = normalize(texture(uNormalTexture, fs_in.texCoord).rgb * 2.0 - 1.0);
	vec4 specular = texture(uSpecularTexture, fs_in.texCoord);
	vec3 specularColor = specular.rgb;
	float metallic = specular.a;
	vec3 emission = texture(uEmissionTexture, fs_in.texCoord).rgb;
	bool hasEmission = (emission.x > 0.0) || (emission.y > 0.0) || (emission.z > 0.0);


	// Calculate Indirect contribution
	float minLevel = calcMinLevel(worldPos);
	vec3 startPos = worldPos; // TODO(snowapril) : calculate startpos more accurately
	vec4 indirectContribution = vec4(0.0, 0.0, 0.0, 1.0);
	for (int i = 0; i < DIFFUSE_CONE_COUNT; ++i)
	{
		indirectContribution += traceCone(startPos, DIFFUSE_CONE_DIRECTIONS[i], DIFFUSE_CONE_APERTURE,
									  	  MAX_TRACE_DISTANCE, minLevel);
	}

	//// TODO
	indirectContribution = clamp(indirectContribution, 0.0, 1.0);

	vec3 directContribution = vec3(0.0);
	if (hasEmission)
	{
		directContribution += emission;
	}
	else
	{
		// calculate Microfacet BRDF model
		//! Roughness is authored as perceptual roughness; as is convention
		//! convert to material roughness by squaring the perceptual roughness [2].
		float alphaRoughness = perceptualRoughness * perceptualRoughness;
		
		//! For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
		//! For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflectance to 0%;
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

		// float visibility = calcShadowCoefficientPCF16(uShadowMaps, worldPos);
		float visibility = calcVisibility(uShadowMaps, uDirectionalLightShadow, worldPos);

		// directContribution += microfacetBRDF(pbr);
		directContribution += microfacetBRDF(pbr) * visibility;
	}

	directContribution = clamp(directContribution, 0.0, 1.0);

	vec3 specularContribution = vec3(0.0);

	vec4 finalColor = vec4(0.0, 0.0, 0.0, 1.0);
	// Ambient occlusion
	directContribution *= indirectContribution.a;
	// Direct contribution
	finalColor.rgb += directContribution;
	// Indirect diffuse contribution
	finalColor.rgb += indirectContribution.rgb;
	// Indirect specular contribution
	finalColor.rgb += specularContribution;

	finalColor = clamp(finalColor, 0.0, 1.0);

	switch(uRenderingMode)
	{
	case DEBUG_DIFFUSE_ONLY:
		fragColor = vec4(diffuse.rgb, 1.0);
		break;
	case DEBUG_SPECULAR_ONLY:
		fragColor = vec4(specularColor, 1.0);
		break;
	case DEBUG_ROUGHNESS_ONLY:
		fragColor = vec4(vec3(perceptualRoughness), 1.0);
		break;
	case DEBUG_DIRECT_CONTRIBUTION_ONLY:
		fragColor = vec4(directContribution, 1.0);
		break;
	case DEBUG_INDIRECT_CONTRIBUTION_ONLY:
		fragColor = vec4(indirectContribution.rgb, 1.0);
		break;
	case DEBUG_AMBIENT_OCCLUSION_ONLY:
		fragColor = vec4(vec3(indirectContribution.a), 1.0);
		break;
	case DEBUG_GI_OUTPUT:
		fragColor = finalColor;
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

vec4 sampleClipmap(sampler3D clipmap, vec3 worldPos, int clipmapLevel, ivec3 faceIndex, vec3 weight)
{
	float voxelSize = uVoxelSize * exp2(clipmapLevel);
	float extent = voxelSize * uVolumeDimension;

	vec3 samplePos = (fract(worldPos / extent) * uVolumeDimension + vec3(BORDER_WIDTH)) / (float(uVolumeDimension) + 2.0 * BORDER_WIDTH);
	// vec3 samplePos = fract(posW / extent); without texture border

	samplePos.y += clipmapLevel;
	samplePos.y /= CLIP_LEVEL_COUNT;
	samplePos.x /= VOXEL_FACE_COUNT;

	return clamp(
		texture(clipmap, samplePos + vec3(faceIndex.x, 0.0, 0.0)) * weight.x +
		texture(clipmap, samplePos + vec3(faceIndex.y, 0.0, 0.0)) * weight.y +
		texture(clipmap, samplePos + vec3(faceIndex.z, 0.0, 0.0)) * weight.z,
		0.0,
		1.0
	);
}

vec4 sampleClipmapLinear(sampler3D clipmap, vec3 worldPos, float curLevel, ivec3 faceIndex, vec3 weight)
{
	int lowerLevel = int(floor(curLevel));
	int upperLevel = int( ceil(curLevel));

	ivec3 faceOffset = faceIndex / VOXEL_FACE_COUNT;
	vec4 lowerSample = sampleClipmap(clipmap, worldPos, lowerLevel, faceOffset, weight);
	vec4 upperSample = sampleClipmap(clipmap, worldPos, upperLevel, faceOffset, weight);

	return mix(lowerSample, upperSample, fract(curLevel));
}

vec4 traceCone(vec3 startPos, vec3 direction, float aperture, float maxDistance, float startLevel)
{
	vec4 result = vec4(0.0);
	float coneCoefficient = 2.0 * tan(aperture * 0.5);

	float curLevel = startLevel;
	float voxelSize = uVoxelSize * exp2(curLevel);

	startPos += direction * voxelSize * 0.5; // TODO(snowapril) : direction * voxelSize * uTraceStartOffset * 0.5; add trace start offset

	float step 		= 0.0;
	float diameter 	= max(step * coneCoefficient, uVoxelSize);

	float stepFactor = MIN_TRACE_STEP_FACTOR; // TODO(snowapril) : max(MIN_TRACE_STEP_FACTOR, uStepFactor);
	float occlusion  = 0.0;

	ivec3 faceIndex = calculateVoxelFaceIndex(direction);
	vec3  weight 	= direction * direction;

	float curSegmentLength 	= voxelSize;
	float minRadius 		= uVoxelSize * uVolumeDimension * 0.5;

	for( ; step < maxDistance && occlusion < 1.0; )
	{
		vec3  position 				= startPos + direction * step;
		float distanceToVoxelCenter = length(uVolumeCenter - position);
		float minLevel 				= ceil(log2(distanceToVoxelCenter / minRadius));

		curLevel = log2(diameter / uVoxelSize);
		curLevel = min(max(max(startLevel, curLevel), minLevel), CLIP_LEVEL_COUNT - 1);

		vec4 radiance = sampleClipmapLinear(uVoxelRadiance, position, curLevel, faceIndex, weight);
		float opacity = sampleClipmapLinear(uVoxelOpacity,  position, curLevel, faceIndex, weight).a;

		voxelSize = uVoxelSize * exp2(curLevel);

		float correction = curSegmentLength / voxelSize;
		radiance.rgb = radiance.rgb * correction;
		opacity = clamp(1.0 - pow(1.0 - opacity, correction), 0.0, 1.0);

		vec4 src = vec4(radiance.rgb, opacity);
		// Alpha blending
		result 	  += clamp(1.0 - result.a, 0.0, 1.0) * src;
		occlusion += (1.0 - occlusion) * opacity / (1.0 + (step + voxelSize) * 1.0); // TODO(snowapril) : (1.0 + (step + voxelSize) * uOcclusionDecay)

		float sLast = step;
		step += max(diameter, uVoxelSize) * stepFactor;
		curSegmentLength = (step - sLast);
		diameter = step * coneCoefficient;
	}

	return clamp(vec4(result.rgb, 1.0 - occlusion), 0.0, 1.0);
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
	
	vec4 minLevelColor = vec4(0.0);
    
    if (minLevel < 1)
        minLevelColor = mix(colors[0], colors[1], fract(minLevel));
    else if (minLevel < 2)
        minLevelColor = mix(colors[1], colors[2], fract(minLevel));
    else if (minLevel < 3)
        minLevelColor = mix(colors[2], colors[3], fract(minLevel));
    else if (minLevel < 4)
        minLevelColor = mix(colors[3], colors[4], fract(minLevel));
    else if (minLevel < 5)
        minLevelColor = mix(colors[4], colors[5], fract(minLevel));
    else if (minLevel < 6)
        minLevelColor = mix(colors[5], colors[6], fract(minLevel));
        
	return minLevelColor * 0.5;
}