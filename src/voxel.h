#ifndef VOXEL_H
#define VOXEL_H

#include <cstdint>

struct VoxelFragmentStruct
{
    // details don't matter
    std::int32_t data[4];
};

struct OctreeNodeStruct
{
    unsigned int id;
};

struct VoxelNodeInfo
{
    std::uint32_t data[16];
};

#endif // VOXEL_H
