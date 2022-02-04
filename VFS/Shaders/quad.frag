#version 450

layout (location = 0) in vec2 aTexCoord;
layout (location = 0) out vec4 fragColor;

layout ( set = 0, binding = 0 ) uniform sampler2D uColorAttachments[4];

layout ( push_constant ) uniform PushConstants
{
	uint attachmentIndex;
} uPushConstants;

void main()
{
	fragColor = vec4(texture(uColorAttachments[uPushConstants.attachmentIndex], aTexCoord).rgb, 1.0);
}