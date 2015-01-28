#version 440 core

in vec2 vsTexCoord;
in vec4 vsOffset[3];

#include "smaa.glsl"

//layout(location = 0) out vec2 outFragColor;
layout(location = 0) out vec4 outFragColor;

layout(binding = 0) uniform sampler2D uInputTex;

void main()
{
    outFragColor = vec4(SMAALumaEdgeDetectionPS(vsTexCoord, vsOffset, uInputTex), 0.0, 0.0);
}
