#version 440 core

in vec3 vsTexCoord;
in vec4 vsOffset[3];

#include "smaa.glsl"

layout(location = 0) out vec2 outFragColor;

layout(binding = 0) uniform sampler2D uInputTex;

void main()
{
    outFragColor = SMAALumaEdgeDetection(vsTexCoord, vsOffset, uInputTex);
}
