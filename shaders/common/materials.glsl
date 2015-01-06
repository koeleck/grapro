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
    int     hasDiffuseTex;

    vec3    specularColor;
    int     hasSpecularTex;

    vec3    ambientColor;
    int     hasAmbientTex;

    vec3    emissiveColor;
    int     hasEmissiveTex;

    vec3    transparentColor;
    int     hasAlphaTex;

    int     hasGlossyTex;
    int     hasNormalTex;
    float   glossiness;
    float   opacity;
};

layout(std430, binding = MATERIAL_BINDING) restrict readonly buffer MaterialBlock
{
    Material    materials[];
};

#endif // SHADER_COMMON_MATERIALS_H
