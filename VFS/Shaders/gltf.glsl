#if !defined(GLTF_GLSL)
#define GLTF_GLSL

#if defined(__cplusplus)
using namespace glm;
#endif

struct GltfShadeMaterial
{
	// PbrMetallicRoughness
	vec4  pbrBaseColorFactor;			// 16
	int   pbrBaseColorTexture;			// 20
	float pbrMetallicFactor;			// 24
	float pbrRoughnessFactor;			// 28 
	int   pbrMetallicRoughnessTexture;	// 32
	int   emissiveTexture;				// 36
	int   alphaMode;					// 40
	float alphaCutoff;					// 56
	int   doubleSided;					// 60
	vec3  emissiveFactor;				// 52
	int   normalTexture;				// 64
	float normalTextureScale;			// 68
	int   occlusionTexture;				// 72
	float occlusionTextureStrength;		// 76
	int padding;						// 80 
};

#endif