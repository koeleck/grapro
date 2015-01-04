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
    uint    diffuseTex;
    uint    specularTex;
    uint    glossyTex;
    uint    normalTex;
    uint    emissiveTex;
    uint    alphaTex;
    float   diffuseColor;
    float   specularColor;
    float   ambientColor;
    float   emissiveColor;
    float   transparentColor;
    float   glossiness;
    float   opacity;
};

layout(std430, binding = MATERIAL_BINDING) restrict readonly buffer MaterialBlock
{
    Material    materials[];
};

#endif  SHADER_COMMON_MATERIALS_H
