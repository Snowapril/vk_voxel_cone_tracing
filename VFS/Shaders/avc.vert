#version 450

layout(location = 0)  in uvec3 aPosition;
layout(location = 0) out VS_OUT { uint level; uint color; };

void main()
{
    vec3 position = vec3(
        aPosition.x >> 16 & 0xffff,
        aPosition.x & 0xffff,
        aPosition.y >> 16 & 0xffff
    );
    level = aPosition.y & 0xffff;
    color = aPosition.z;
    gl_Position = vec4(position, 1.0);
}