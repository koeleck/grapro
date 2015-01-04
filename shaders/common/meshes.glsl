#ifndef SHADER_COMMON_MESHES_GLSL
#define SHADER_COMMON_MESHES_GLSL

#include "bindings.glsl"

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!                                                 !!
//!!  keep in sync with src/core/shader_interface.h  !!
//!!                                                 !!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

struct Mesh
{
    uint    first;
    uint    stride;

    // (0) normal (1) texcoord (2) tangent (3) vertex color
    bvec4   components;
};

#define MESH_COMPONENT_NORMAL       0
#define MESH_COMPONENT_TEXCOORD     1
#define MESH_COMPONENT_TANGENT      2
#define MESH_COMPONENT_VERTEX_COLOR 3

layout(std430, binding = MESH_BINDING) restrict readonly buffer MeshBlock
{
    Mesh    meshes[];
};

#endif // SHADER_COMMON_MESHES_GLSL
