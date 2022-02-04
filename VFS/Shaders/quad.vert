#version 450

layout (location = 0) out vec2 texCoord;

layout ( push_constant ) uniform PushConstants
{
	uint attachmentIndex;
} uPushConstants;

void main()
{
	vec2 pos = vec2(gl_VertexIndex & 1, (gl_VertexIndex & 2) >> 1) * 2 - 1;
	texCoord = pos.xy * 0.5 + 0.5;
	pos.xy = (pos.xy * 0.5) + (vec2(uPushConstants.attachmentIndex & 1, (uPushConstants.attachmentIndex & 2) >> 1) - 0.5);
	gl_Position = vec4(pos.xy, 0, 1);
}
