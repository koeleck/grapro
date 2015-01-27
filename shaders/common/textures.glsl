#ifndef SHADERS_COMMON_TEXTURES_GLSL
#define SHADERS_COMMON_TEXTURES_GLSL

#include "bindings.glsl"

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!                                                 !!
//!!  keep in sync with src/core/shader_interface.h  !!
//!!                                                 !!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


layout(binding = DIFFUSE_TEX_UNIT) uniform sampler2D uDiffuseTex;
layout(binding = SPECULAR_TEX_UNIT) uniform sampler2D uSpecularTex;
layout(binding = GLOSSY_TEX_UNIT) uniform sampler2D uGlossyTex;
layout(binding = NORMAL_TEX_UNIT) uniform sampler2D uNormalTex;
layout(binding = EMISSIVE_TEX_UNIT) uniform sampler2D uEmissiveTex;
layout(binding = ALPHA_TEX_UNIT) uniform sampler2D uAlphaTex;
layout(binding = AMBIENT_TEX_UNIT) uniform sampler2D uAmbientTex;
layout(binding = SHADOWMAP_TEX_UNIT) uniform sampler2DArrayShadow uShadowMapTex;
layout(binding = SHADOWCUBEMAP_TEX_UNIT) uniform samplerCubeArrayShadow uShadowCubeMapTex;


#endif // SHADERS_COMMON_TEXTURES_GLSL
