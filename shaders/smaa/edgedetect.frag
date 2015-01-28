#version 440 core

in vec2 vsTexCoord;
in vec4 vsOffset[3];

#include "smaa.glsl"

layout(location = 0) out vec4 outEdges;

layout(binding = 0) uniform sampler2D uInputTex;
layout(binding = 1) uniform sampler2D uDepthTex;

void main()
{
    //outEdges = vec4(SMAADepthEdgeDetectionPS(vsTexCoord, vsOffset, uDepthTex), 0.0, 0.0);
    //outEdges = vec4(SMAAColorEdgeDetectionPS(vsTexCoord, vsOffset, uInputTex), 0.0, 0.0);
    outEdges = vec4(SMAALumaEdgeDetectionPS(vsTexCoord, vsOffset, uInputTex), 0.0, 0.0);
}
