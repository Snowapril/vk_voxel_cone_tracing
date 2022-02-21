#version 450

layout( triangles ) in;
layout( triangle_strip, max_vertices = 3 ) out;

layout( location = 0 ) in VS_OUT {
	vec2 texCoord;
	vec3 normal;
} gs_in[];

layout( location = 0 ) out GS_OUT {
	vec3 position;
	vec2 texCoord;
	vec3 normal;
} gs_out;

layout ( std140, set = 3, binding = 0 ) uniform ViewProjection {  
	mat4 uViewProj[3]; 
	mat4 uViewProjIt[3]; 
};

vec2 project(vec3 vertex, uint axis) 
{
	return axis == 0 ? vertex.yz : (axis == 1 ? vertex.xz : vertex.xy);
}

vec3 biasAndScale(vec3 vertex)
{
	return (vertex + 1.0) * 0.5;
}

int getDominantAxis(vec3 pos0, vec3 pos1, vec3 pos2)
{
	vec3 normal = abs(cross(pos1 - pos0, pos2 - pos0));
	return (normal.x > normal.y && normal.x > normal.z) ? 0 : 
			(normal.y > normal.z) ? 1 : 2;
}

void main()
{
	int axis = getDominantAxis(gl_in[0].gl_Position.xyz,
							   gl_in[1].gl_Position.xyz,
							   gl_in[2].gl_Position.xyz);
	

	for (int i = 0; i < 3; ++i)
	{	
		gl_ViewportIndex 	= axis;
		gl_Position 		= vec4(project(gl_in[i].gl_Position.xyz, axis), 1.0, 1.0); // uViewProj[axis] * gl_in[i].gl_Position;
		gs_out.texCoord 	= gs_in[i].texCoord;
		gs_out.position 	= biasAndScale(gl_in[i].gl_Position.xyz); // gl_in[i].gl_Position.xyz;
		gs_out.normal 		= gs_in[i].normal;
		EmitVertex();
	}
	EndPrimitive();
}

// #version 450
// 
// layout( triangles ) in;
// layout( triangle_strip, max_vertices=3 ) out;
// 
// layout( location = 0 ) in VS_OUT {
//     vec2 texCoord;
//     vec3 normal;
// } gs_in[];
// 
// layout( location = 0 ) out GS_OUT {
//     vec3 position;
//     vec3 normal;
//     vec2 texCoord;
// } gs_out;
// 
// vec2 project(vec3 vertex, uint axis) 
// {
// 	return axis == 0 ? vertex.yz : (axis == 1 ? vertex.xz : vertex.xy);
// }
// 
// vec3 biasAndScale(vec3 vertex)
// {
// 	return (vertex + 1.0) * 0.5;
// }
// 
// void main()
// {
// 	vec3 p0 = gl_in[0].gl_Position.xyz;
// 	vec3 p1 = gl_in[1].gl_Position.xyz;
// 	vec3 p2 = gl_in[2].gl_Position.xyz;
// 
// 	vec3 normal = abs(cross(p1 - p0, p2 - p0));
// 	uint axis = (normal.x > normal.y && normal.x > normal.z) ? 
// 								   0 : (normal.y > normal.z) ? 
// 								   1 : 2;
// 	
// 	gs_out.texCoord = aTexCoord[0];
// 	gs_out.position = biasAndScale(p0);
// 	gl_Position = vec4(project(p0, axis), 1.0, 1.0);
// 	EmitVertex();
// 	
// 	gs_out.texCoord = aTexCoord[1];
// 	gs_out.position = biasAndScale(p1);
// 	gl_Position = vec4(project(p1, axis), 1.0, 1.0);
// 	EmitVertex();
// 
// 	gs_out.texCoord = aTexCoord[2];
// 	gs_out.position = biasAndScale(p2);
// 	gl_Position = vec4(project(p2, axis), 1.0, 1.0);
// 	EmitVertex();
// 
// 	EndPrimitive();
// }