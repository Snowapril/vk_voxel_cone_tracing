#version 450 core

layout(points) in;
layout(triangle_strip, max_vertices = 14) out;
// layout(points, max_vertices = 1) out;

layout(location = 0) in VS_OUT
{
	uint level;
	uint color;
} gs_in[];

layout(location = 1) out GS_OUT { vec4 color; };

layout ( set = 0, binding = 0 ) uniform CamMatrix
{ 
 	mat4 viewProj;
 	mat4 viewProjInv;
} uCamMatrix;

layout ( push_constant ) uniform PushConstants
{
	uint maxLevel;
} uPushConstants;

vec4 convRGBA8ToVec4( uint val) 
{	
	return vec4 (
		float (( val & 0x000000ff )), 
		float (( val & 0x0000ff00 ) >>  8u ),
		float (( val & 0x00ff0000 ) >> 16u ),
		float (( val & 0xff000000 ) >> 24u )
	);
}

void emit(in vec3 position)
{
	gl_Position = uCamMatrix.viewProj * vec4(position, 1.0);
	color = vec4(position, 1.0); // convRGBA8ToVec4(gs_in[0].color) / 255.0;
    gl_PointSize = 32;
	EmitVertex();
}

vec3 scaleAndBias(vec3 pos, float invHalfMaxScale)
{
	return pos * invHalfMaxScale - 1.0;
}

void main()
{
	float invHalfMaxScale = 2.0 / (1 << uPushConstants.maxLevel);
	uint resolution = 1 << gs_in[0].level;
	vec3 center = gl_in[0].gl_Position.xyz;
	vec3 llf = center - resolution * 0.5;
	vec3 urb = center + resolution * 0.5;
	//! Emit triangle strip for generating cube
	// emit(scaleAndBias(center, invHalfMaxScale));
	emit(scaleAndBias(vec3(llf.x, urb.y, urb.z), invHalfMaxScale));
	emit(scaleAndBias(vec3(urb.x, urb.y, llf.z), invHalfMaxScale));
	emit(scaleAndBias(vec3(urb.x, urb.y, urb.z), invHalfMaxScale));
	emit(scaleAndBias(vec3(urb.x, llf.y, urb.z), invHalfMaxScale));
	emit(scaleAndBias(vec3(llf.x, urb.y, urb.z), invHalfMaxScale));
	emit(scaleAndBias(vec3(llf.x, llf.y, urb.z), invHalfMaxScale));
	emit(scaleAndBias(vec3(llf.x, urb.y, llf.z), invHalfMaxScale));
	emit(scaleAndBias(vec3(llf.x, llf.y, llf.z), invHalfMaxScale));
	emit(scaleAndBias(vec3(urb.x, urb.y, llf.z), invHalfMaxScale));
	emit(scaleAndBias(vec3(urb.x, llf.y, llf.z), invHalfMaxScale));
	emit(scaleAndBias(vec3(urb.x, llf.y, urb.z), invHalfMaxScale));
	emit(scaleAndBias(vec3(llf.x, llf.y, llf.z), invHalfMaxScale));
	emit(scaleAndBias(vec3(llf.x, llf.y, urb.z), invHalfMaxScale));
	EndPrimitive();
}