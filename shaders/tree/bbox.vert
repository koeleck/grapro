#version 440 core

#include "common/camera.glsl"

layout(location = 0) uniform vec3 bbox[2];

void main()
{
    vec4 pos = vec4(bbox[(gl_VertexID>>0) & 0x01].x,
                    bbox[(gl_VertexID>>1) & 0x01].y,
                    bbox[(gl_VertexID>>2) & 0x01].z,
                    1.0);

    gl_Position = cam.ProjViewMatrix * pos;
}
