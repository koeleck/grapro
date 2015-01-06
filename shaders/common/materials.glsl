#ifndef SHADER_COMMON_MATERIALS_H
#define SHADER_COMMON_MATERIALS_H

#include "bindings.glsl"

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!                                                 !!
//!!  keep in sync with src/core/shader_interface.h  !!
//!!                                                 !!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

struct Material
{
    vec3    diffuseColor;
    vec3    specularColor;
    vec3    ambientColor;
    vec3    emissiveColor;
    vec3    transparentColor;
    uvec2   diffuseTex;
    uvec2   specularTex;
    uvec2   glossyTex;
    uvec2   normalTex;
    uvec2   emissiveTex;
    uvec2   alphaTex;
    uvec2   ambientTex;
    float   glossiness;
    float   opacity;
    //uint    diffuseTex;
    //uint    specularTex;
    //uint    glossyTex;
    //uint    normalTex;
    //uint    emissiveTex;
    //uint    alphaTex;
};

layout(std430, binding = MATERIAL_BINDING) restrict readonly buffer MaterialBlock
{
    Material    materials[];
};

#endif // SHADER_COMMON_MATERIALS_H
