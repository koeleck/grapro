#ifndef SHADERS_COMMON_TEXTURES_GLSL
#define SHADERS_COMMON_TEXTURES_GLSL

#include "bindings.glsl"

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!                                                 !!
//!!  keep in sync with src/core/shader_interface.h  !!
//!!                                                 !!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define DIFFUSE_TEX_UNIT    0
#define SPECULAR_TEX_UNIT   1
#define GLOSSY_TEX_UNIT     2
#define NORMAL_TEX_UNIT     3
#define EMISSIVE_TEX_UNIT   4
#define ALPHA_TEX_UNIT      5
#define AMBIENT_TEX_UNIT    6


layout(binding = DIFFUSE_TEX_UNIT) uniform sampler2D uDiffuseTex;
layout(binding = SPECULAR_TEX_UNIT) uniform sampler2D uSpecularTex;
layout(binding = GLOSSY_TEX_UNIT) uniform sampler2D uGlossyTex;
layout(binding = NORMAL_TEX_UNIT) uniform sampler2D uNormalTex;
layout(binding = EMISSIVE_TEX_UNIT) uniform sampler2D uEmissiveTex;
layout(binding = ALPHA_TEX_UNIT) uniform sampler2D uAlphaTex;
layout(binding = AMBIENT_TEX_UNIT) uniform sampler2D uAmbientTex;


#endif // SHADERS_COMMON_TEXTURES_GLSL
