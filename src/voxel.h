#ifndef VOXEL_H
#define VOXEL_H

#include <glm/glm.hpp>

struct VoxelStruct
{
    glm::uvec4 position;
    glm::vec4 color;
    glm::vec4 normal;
};

struct OctreeNodeStruct
{
    unsigned int id;
};

struct OctreeNodeColorStruct
{
    unsigned int color;
};

#endif // VOXEL_H
