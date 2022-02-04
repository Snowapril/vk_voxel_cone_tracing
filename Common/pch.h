// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include <vector>

#ifndef OUT
#define OUT
#endif

#pragma warning (push)
#pragma warning (disable :  4191)
#pragma warning (disable :  4201)
#pragma warning (disable :  4296)
#pragma warning (disable :  4324)
#pragma warning (disable :  4355)
#pragma warning (disable :  4464)
#pragma warning (disable :  4701)
#pragma warning (disable :  4819)
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
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#pragma warning (pop)

#endif //PCH_H
