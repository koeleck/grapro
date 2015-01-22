#version 440 core

#include "common/extensions.glsl"
#include "common/camera.glsl"

layout(location = 0) uniform vec3 center;

out vec4 wpos;

void main()
{
    gl_Position = vec4(center, 1);
    wpos = vec4(center, 1);
}
