#version 440 core

out vec2 vsTexCoord;
out vec2 vsPixCoord;
out vec4 vsOffset[3];

#include "smaa.glsl"


void main()
{
    vsTexCoord = vec2(float(gl_VertexID & 0x01) * 2.0,
                      float(gl_VertexID & 0x02));

    SMAABlendingWeightCalculationVS(vsTexCoord,
            vsPixCoord, vsOffset);

    gl_Position = vec4(fma(vsTexCoord, vec2(2.0), vec2(-1.0)),
                       -1.0,
                       1.0);
}

