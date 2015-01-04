#ifndef SHADER_COMMON_INSTANCES_GLSL
#define SHADER_COMMON_INSTANCES_GLSL

#include "bindings.glsl"

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!                                                 !!
//!!  keep in sync with src/core/shader_interface.h  !!
//!!                                                 !!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

struct Instance
{
    mat4    modelMatrix;
    uint    meshID;
    uint    materialID;
};

layout(std430, binding = INSTANCE_BINDING) restrict readonly buffer InstanceBlock
{
    Instance    instances[];
};

#endif // SHADER_COMMON_INSTANCES_GLSL
