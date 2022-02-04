#if !defined(LIGHT_GLSL)
#define LIGHT_GLSL

#if defined(__cplusplus)
using namespace glm;
#endif

struct DirectionalLightDesc {
	vec3 direction;   // 12
	float intensity;  // 16
	vec3 color;		  // 28
	int padding;	  // 32
};

struct DirectionalLightShadowDesc {
	mat4 view;        // 16
	mat4 proj;		  // 32
	// float zNear;	  // 36
	// float zFar;		  // 40
	// float pcfRadius;  // 44
	// int padding;	  // 48
};

#endif