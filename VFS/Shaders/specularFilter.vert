#version 450

layout (location = 0) out VS_OUT {
	vec2 texCoord;
} vs_out;

void main()
{
	vec2 pos = vec2(gl_VertexIndex & 1, (gl_VertexIndex & 2) >> 1) * 2 - 1;
	vs_out.texCoord = pos.xy * 0.5 + 0.5;
	gl_Position = vec4(pos.xy, 0, 1);
}	