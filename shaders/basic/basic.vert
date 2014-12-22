#version 440 core

#include "common/camera.glsl"

layout(location = 0) in vec3 in_Position;

uniform mat4 PVMat;

void main()
{
    gl_Position = ProjViewMatrix * vec4(in_Position, 1.0);
    //gl_Position = vec4(in_Position, 1.0);
}
