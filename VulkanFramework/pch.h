// Author : Jihong Shin (snowapril)

#if !defined(VULKAN_FRAMEWORK_PCH_H)
#define VULKAN_FRAMEWORK_PCH_H

#include <VulkanFramework/NonCopyable.h>
#include <VulkanFramework/ForwardDeclarations.h>
#include <vector>

#pragma warning (push)
#pragma warning (disable :  4191)
#pragma warning (disable :  4296)
#pragma warning (disable :  4324)
#pragma warning (disable :  4355)
#pragma warning (disable :  4464)
#pragma warning (disable :  4701)
#pragma warning (disable :  4819)
#pragma warning (disable :  4946)
#pragma warning (disable :  5026)
#pragma warning (disable :  5027)
#pragma warning (disable :  5038)
#pragma warning (disable :  6262)
#pragma warning (disable :  6386)
#pragma warning (disable :  6387)
#pragma warning (disable : 26110)
#pragma warning (disable : 26495)
#pragma warning (disable : 26812)

#define GLM_FORCE_SIZE_T_LENGTH
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_FORCE_LEFT_HANDED

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <tinygltf/tiny_gltf.h>

#pragma warning (pop)

#endif