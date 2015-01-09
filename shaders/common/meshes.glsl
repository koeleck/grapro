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
    uint    stride;
    uint    components;
    uint    first;
    uint    firstIndex;
    uint    count;
};

// keep in sync with core/mesh.h: MeshComponents
#define MESH_COMPONENT_TEXCOORD     0x01
#define MESH_COMPONENT_NORMAL       0x02
#define MESH_COMPONENT_TANGENT      0x04

layout(std430, binding = MESH_BINDING) restrict readonly buffer MeshBlock
{
    Mesh    meshes[];
};

#endif // SHADER_COMMON_MESHES_GLSL
