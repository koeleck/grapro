#version 440 core

in vec2 vsTexCoord;
in vec2 vsPixCoord;
in vec4 vsOffset[3];

#include "smaa.glsl"

layout(location = 1) out vec4 outBlendTex;

layout(binding = 1) uniform sampler2D uEdgesTex;
layout(binding = 2) uniform sampler2D uAreaTex;
layout(binding = 3) uniform sampler2D uSearchTex;

void main()
{
    outBlendTex = SMAABlendingWeightCalculationPS(vsTexCoord,
            vsPixCoord, vsOffset,
            uEdgesTex, uAreaTex, uSearchTex,
            vec4(0.0));
}

