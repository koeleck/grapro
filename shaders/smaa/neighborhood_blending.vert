#version 440 core

out vec2 vsTexCoord;
out vec4 vsOffset;

#include "smaa.glsl"

void main()
{
    const vec2 texcoord = vec2(float(gl_VertexID & 0x01) * 2.0,
                               float(gl_VertexID & 0x02));

    vsTexCoord = texcoord;
    vec4 off;
    SMAANeighborhoodBlendingVS(texcoord, off);
    vsOffset = off;

    gl_Position = vec4(fma(texcoord, vec2(2.0), vec2(-1.0)),
                       -1.0,
                       1.0);
}


