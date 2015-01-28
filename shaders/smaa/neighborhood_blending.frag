#version 440 core

in vec2 vsTexCoord;
in vec4 vsOffset;

#include "smaa.glsl"

layout(location = 0) out vec4 outFragColor;

layout(binding = 0) uniform sampler2D uInputTex;
layout(binding = 1) uniform sampler2D uBlendTex;

void main()
{
    outFragColor = SMAANeighborhoodBlendingPS(vsTexCoord, vsOffset,
            uInputTex, uBlendTex);
}

