#version 450 core

layout(points) in;
layout(triangle_strip, max_vertices = 24) out;

layout(location = 0) out GS_OUT { 
	vec2 texCoord;
	vec4 color;
} gs_out;

layout ( set = 0, binding = 0 ) uniform CamMatrix
{ 
 	mat4 viewProj;
 	mat4 viewProjInv;
} uCamMatrix;

layout ( set = 1, binding = 0 ) uniform sampler3D uVoxelOpacity;

layout ( push_constant ) uniform PushConstants
{
	float voxelSize;
} uPushConstants;

void emitQuad(in vec4 p0, in vec4 p1, in vec4 p2, in vec4 p3, in vec4 color)
{
	gl_Position = p0;
	gs_out.texCoord = vec2(0, 0);
	gs_out.color = color;
	EmitVertex();
	gl_Position = p1;
	gs_out.texCoord = vec2(0, 1);
	gs_out.color = color;
	EmitVertex();
	gl_Position = p3;
	gs_out.texCoord = vec2(1, 0);
	gs_out.color = color;
	EmitVertex();
	gl_Position = p2;
	gs_out.texCoord = vec2(1, 1);
	gs_out.color = color;
	EmitVertex();
	EndPrimitive();
}

void main()
{
	ivec3 pos = ivec3(gl_in[0].gl_Position.xyz);

	// ivec3 posV = pos + u_regionMin;
	
	// Expecting u_prevRegionMin and u_prevVolumeMax in the voxel coordinates of the current region
	// if (u_hasPrevClipmapLevel > 0 && (all(greaterThanEqual(posV, u_prevRegionMin)) && all(lessThan(posV, u_prevRegionMax))))
	// 	return;

	ivec3 u_imageMin = ivec3(0, 0, 3); // todo
	int u_clipmapResolution = 128; // todo
	int BORDER_WIDTH = 1; // todo
	int u_clipmapLevel = 0; // todo

	ivec3 samplePos = (u_imageMin + pos) & (u_clipmapResolution - 1);
	int resolution = u_clipmapResolution + BORDER_WIDTH * 2;
	samplePos += ivec3(BORDER_WIDTH);
	
	// Target correct clipmap level
	samplePos.y += u_clipmapLevel * resolution;
		
	vec4 colors[6] = vec4[6] (
		texelFetch(uVoxelOpacity, samplePos, 0),
		texelFetch(uVoxelOpacity, samplePos + ivec3(resolution, 0, 1), 0),
		texelFetch(uVoxelOpacity, samplePos + ivec3(2 * resolution, 0, 1), 0),
		texelFetch(uVoxelOpacity, samplePos + ivec3(3 * resolution, 0, 1), 0),
		texelFetch(uVoxelOpacity, samplePos + ivec3(4 * resolution, 0, 1), 0),
		texelFetch(uVoxelOpacity, samplePos + ivec3(5 * resolution, 0, 1), 0));

	vec4 v0 = uCamMatrix.viewProj * vec4( uPushConstants.voxelSize * vec3(pos), 1.0);
	vec4 v1 = uCamMatrix.viewProj * vec4( uPushConstants.voxelSize * vec3(pos + ivec3(1, 0, 0)), 1.0);
	vec4 v2 = uCamMatrix.viewProj * vec4( uPushConstants.voxelSize * vec3(pos + ivec3(0, 1, 0)), 1.0);
	vec4 v3 = uCamMatrix.viewProj * vec4( uPushConstants.voxelSize * vec3(pos + ivec3(1, 1, 0)), 1.0);
	vec4 v4 = uCamMatrix.viewProj * vec4( uPushConstants.voxelSize * vec3(pos + ivec3(0, 0, 1)), 1.0);
	vec4 v5 = uCamMatrix.viewProj * vec4( uPushConstants.voxelSize * vec3(pos + ivec3(1, 0, 1)), 1.0);
	vec4 v6 = uCamMatrix.viewProj * vec4( uPushConstants.voxelSize * vec3(pos + ivec3(0, 1, 1)), 1.0);
	vec4 v7 = uCamMatrix.viewProj * vec4( uPushConstants.voxelSize * vec3(pos + ivec3(1, 1, 1)), 1.0);

	emitQuad(v0, v2, v6, v4, colors[0]);
	emitQuad(v1, v5, v7, v3, colors[1]);
	emitQuad(v0, v4, v5, v1, colors[2]);
	emitQuad(v2, v3, v7, v6, colors[3]);
	emitQuad(v0, v1, v3, v2, colors[4]);
	emitQuad(v4, v6, v7, v5, colors[5]);
}