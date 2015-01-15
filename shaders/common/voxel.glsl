#ifndef SHADER_COMMON_VOXEL_GLSL
#define SHADER_COMMON_VOXEL_GLSL

#include "bindings.glsl"

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!                                                 !!
//!!  keep in sync with src/voxel.h  				 !!
//!!                                                 !!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

struct octreeBuffer
{
    uint    id;
};

layout(std430, binding = OCTREE_BINDING) restrict buffer octreeBlock
{
    octreeBuffer octree[];
};

#endif // SHADER_COMMON_VOXEL_GLSL