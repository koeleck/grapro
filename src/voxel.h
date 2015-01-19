#ifndef VOXEL_H
#define VOXEL_H

#include <glm/glm.hpp>

struct VoxelStruct
{
    unsigned int position; // 2 bits unused, 10/10/10 bits pos.x/y/z
    unsigned int color; // 11/11/10
    unsigned int padding, padding2;
};

struct OctreeNodeStruct
{
    unsigned int id;
};

struct OctreeNodeColorStruct
{
    glm::vec4 color;
};

#endif // VOXEL_H
