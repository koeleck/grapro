#version 440 core

in vec2 vsTexCoord;
in vec2 vsPixCoord;
in vec4 vsOffset[3];

#include "smaa.glsl"

layout(location = 0) out vec4 outFragColor;

layout(binding = 0) uniform sampler2D uEdgesTex;
layout(binding = 1) uniform sampler2D uAreaTex;
layout(binding = 2) uniform sampler2D uSearchTex;

void main()
{
    outFragColor = SMAABlendingWeightCalculationPS(vsTexCoord,
            vsPixCoord, vsOffset,
            uEdgesTex, uAreaTex, uSearchTex,
            vec4(0.0));
}

