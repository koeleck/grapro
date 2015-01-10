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

#endif // VOXEL_H
