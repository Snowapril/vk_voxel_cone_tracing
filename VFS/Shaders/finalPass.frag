#version 450

layout (location = 0) in VS_OUT {
	vec2 texCoord;
} fs_in;

layout (location = 0) out vec4 fragColor;

layout ( set = 0, binding = 0 ) uniform sampler2D uFinalOutputAttachment;

void main()
{
	fragColor = texture(uFinalOutputAttachment, fs_in.texCoord);
}	