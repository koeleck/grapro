#ifndef SHADER_COMMON_VERTICES_GLSL
#define SHADER_COMMON_VERTICES_GLSL

#include "bindings.glsl"

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!                                                 !!
//!!  keep in sync with src/core/shader_interface.h  !!
//!!                                                 !!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

layout(std430, binding = VERTEX_BINDING) restrict readonly buffer VertexBlock
{
    float vertexData[];
};

#endif // SHADER_COMMON_VERTICES_GLSL

