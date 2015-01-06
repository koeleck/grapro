#version 440 core

#include "common/camera.glsl"

uniform vec3 uBBox[2];

void main()
{
    vec4 pos = vec4(uBBox[(gl_VertexID>>0) & 0x01].x,
                    uBBox[(gl_VertexID>>1) & 0x01].y,
                    uBBox[(gl_VertexID>>2) & 0x01].z,
                    1.0);

    gl_Position = cam.ProjViewMatrix * pos;
}
