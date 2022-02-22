#version 450

#include "light.glsl"
#include "brdf.glsl"
#include "shadow.glsl"
#include "tonemapping.glsl"

layout ( constant_id = 0 ) const int MAX_DIRECTIONAL_LIGHT_NUM 	= 8;
layout ( constant_id = 1 ) const int CLIP_LEVEL_COUNT 			= 6;
layout ( constant_id = 2 ) const int VOXEL_FACE_COUNT 			= 6;

layout (location = 0) in VS_OUT {
	vec2 texCoord;
} fs_in;
//layout (location = 0) out vec4 fragColor;
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
layout ( set = 1, binding = 4 ) uniform sampler2D uDepthTexture;

// Sparse voxel octree buffer binding
layout ( std430, set = 2, binding = 0 ) readonly buffer OctreeStorageBuffer { 
    uvec2 uVoxelOctreeNode[]; 
};

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
	//vec3  uSceneWorldBBMin;
	float uVoxelSize;				 // 20
	//vec3  uSceneWorldBBMax;
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
	bool hasEmission 			= (emission.x > 0.0) || (emission.y > 0.0) || (emission.z > 0.0);

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
										  	  MAX_TRACE_DISTANCE, minLevel, max(MIN_TRACE_STEP_FACTOR, uMinTraceStepFactor)) * cosTheta;
			validConeCount += cosTheta;
		}
	}
	else
	{
		for (int i = 0; i < DIFFUSE_CONE_COUNT_32; ++i)
		{
			float cosTheta = dot(normal, DIFFUSE_CONE_DIRECTIONS_32[i]);
			if (cosTheta < 0.0) 
				continue;
	
			indirectContribution += traceCone(startPos, DIFFUSE_CONE_DIRECTIONS_32[i], DIFFUSE_CONE_APERTURE_32,
										  	  MAX_TRACE_DISTANCE, minLevel, max(MIN_TRACE_STEP_FACTOR, uMinTraceStepFactor)) * cosTheta;
			validConeCount += cosTheta;
		}
	}

	indirectContribution /= validConeCount;
	indirectContribution.a *= uAmbientOcclusionFactor;
	indirectContribution.rgb *= diffuseColor * uIndirectDiffuseIntensity;
	indirectContribution = clamp(indirectContribution, 0.0, 1.0);

	// Specular cone
	vec3 indirectSpecularContribution = vec3(0.0);
	if (any(greaterThan(specularColor, vec3(1e-6))) && metallic > 1e-6)
	{
		vec3 specularConeDirection = normalize(reflect(-view, normal));
		indirectSpecularContribution += traceCone(
			startPos, specularConeDirection, 
			max(perceptualRoughness, MIN_SPECULAR_APERTURE), 
			MAX_TRACE_DISTANCE, minLevel, max(MIN_TRACE_STEP_FACTOR, uVoxelSize)
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

	directContribution = clamp(directContribution, 0.0, 1.0);

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
		//diffuseContribution.rgb += directContribution;
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

vec3 worldPosFromDepth(float depth)
{
    vec4 pos = vec4(fs_in.texCoord, depth, 1.0);
    pos.xy = pos.xy * 2.0 - 1.0;
    pos = uViewProjInv * pos;
    return pos.xyz / pos.w;
}

vec4 convRGBA8ToVec4( uint val) {
	return vec4(
	 	float(( val & 0x000000FF )),
	 	float(( val & 0x0000FF00 ) >> 8U ),
	 	float(( val & 0x00FF0000 ) >> 16U),
	 	float(( val & 0xFF000000 ) >> 24U) 
	);
}

vec4 sampleSVO(vec3 position, uint level, vec3 weight)
{
	vec3 uSceneWorldBBMin = vec3(-15.367, -1.011, -9.462); // TODO(snowapril) : must replace this with push_constant
	vec3 uSceneWorldBBMax = vec3(14.399, 11.43, 8.84); // TODO(snowapril) : must replace this with push_constant
	vec3 extent = uSceneWorldBBMax - uSceneWorldBBMin;
    float extentValue = max(extent.x, max(extent.y, extent.z)) * 0.5;
    vec3 center = (uSceneWorldBBMin + uSceneWorldBBMax) * 0.5;

	uint resolution = uint(uVolumeDimension);
	vec3  scaledPos = (position - center) / extentValue;

    uvec3 fragPos = clamp(uvec3(((scaledPos + 1.0) * 0.5) * resolution), 
						  uvec3(0u), 
						  uvec3(resolution));

    uint targetResolution = 1 << level;
	uint idx = 0, cur = 0; 
	uvec3 cmp;
	do {
		resolution >>= 1;
		cmp = uvec3(greaterThanEqual(fragPos, uvec3(resolution)));
		idx = cur + (cmp.z | (cmp.x << 1) | (cmp.y << 2));  // TODO(snowapril) : why rotation work?
		cur = uVoxelOctreeNode[idx].x & 0x7fffffff; // Without MSB flag
		fragPos -= cmp * resolution;
	} while (cur != 0u && resolution > targetResolution);

	return convRGBA8ToVec4(uVoxelOctreeNode[idx].y) / 255.0;
}

vec4 sampleSVOLinear(vec3 position, float level, vec3 weight)
{
	uint lowerLevel = uint(floor(level));
	uint upperLevel = uint( ceil(level));

	vec4 lowerSample = sampleSVO(position, lowerLevel, weight);
	vec4 upperSample = sampleSVO(position, upperLevel, weight);

	return mix(lowerSample, upperSample, fract(level));
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

	vec3  weight 	 = direction * direction;

	float curSegmentLength 	= voxelSize;
	float minRadius 		= uVoxelSize * uVolumeDimension * 0.5;

	for( ; step < maxDistance && occlusion < 1.0; )
	{
		vec3  position 				= startPos + direction * step;
		float distanceToVoxelCenter = length(uVolumeCenter - position);
		float minLevel 				= ceil(log2(distanceToVoxelCenter / minRadius));

		curLevel = log2(diameter / uVoxelSize);
		curLevel = min(max(max(startLevel, curLevel), minLevel), CLIP_LEVEL_COUNT - 1);

		vec4 clipmapSample = sampleSVOLinear(position, curLevel, weight);
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