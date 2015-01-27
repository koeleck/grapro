#ifndef SHADERS_COMMON_GBUFFER_GLSL
#define SHADERS_COMMON_GBUFFER_GLSL

#include "common/camera.glsl"

layout(binding = GBUFFER_DEPTH_TEX) uniform sampler2D uDepthTex;
layout(binding = GBUFFER_DIFFUSE_NORMAL_TEX) uniform sampler2D uDiffuseNormalTex;
layout(binding = GBUFFER_SPECULAR_GLOSSY_EMISSIVE_TEX) uniform sampler2D uSpecularGlossyEmissiveTex;

float edge_filter(in vec2 center, in vec2 a0, in vec2 a1,
                  in vec2 a2, in vec2 a3)
{
    const float THRESH=30.0 / 255.0;

    vec4 lum = vec4(a0.x, a1.x , a2.x, a3.x);
    vec4 w = 1.0 - step(THRESH, abs(lum - center.x));
    float W = w.x + w.y + w.z + w.w;
    //Handle the special case where all the weights are zero.
    //In HDR scenes it's better to set the chrominance to zero.
    //Here we just use the chrominance of the first neighbor.
    w.x = (W == 0.0) ? 1.0 : w.x;
    W = (W == 0.0) ? 1.0 : W;

    return (w.x * a0.y + w.y * a1.y + w.z * a2.y +w.w * a3.y) / W;
    //W = (W == 0.0) ? W : 1.0 / W;
    //return (w.x * a0.y + w.y * a1.y + w.z * a2.y + w.w * a3.y) * W;
}


vec4 resconstructWorldPos(in vec2 texcoord)
{
    float d = texture(uDepthTex, texcoord).r * 2.0 - 1.0;
    vec4 pos = vec4(texcoord * 2.0 - 1.0, d, 1.0);
    pos = cam.InvProjViewMatrix * pos;
    pos /= pos.w;
    return vec4(pos.xyz, 1.0);
}


#endif // SHADERS_COMMON_GBUFFER_GLSL
