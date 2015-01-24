#ifndef SHADERS_TREE_VOXEL_GLSL
#define SHADERS_TREE_VOXEL_GLSL

#include "common/bindings.glsl"

// keep in sync with src/voxel.h

struct VoxelFragment
{
    uint position_xyz;
    uint diff_rgb_normal_x;
    uint normal_yz_emissive_rg;
    uint emissive_b;
};

layout(std430, binding = VOXEL_BINDING) restrict buffer VoxelFragmentBlock
{
    VoxelFragment voxelFragment[];
};


struct OctreeNode
{
    uint    id;
};

layout(std430, binding = OCTREE_BINDING) restrict buffer OctreeNodeBlock
{
    OctreeNode octree[];
};


struct NodeInfo
{
    vec3 position;
    uint x_neighbors_y_neg_neighbor;
    vec3 diffuse;
    uint y_pos_neighbor_z_neighbors;
    vec3 emissive;
    uint count;
    vec4 normal;
};

layout(std430, binding = OCTREE_INFO_BINDING) restrict buffer OctreeInfoBlock
{
    NodeInfo octreeInfo[];
};

#if defined(NUM_BRICKS_X) && defined(NUM_BRICKS_Y)
ivec3 getBrickCoord(in uint idx)
{
    const int iidx = int(idx);
    ivec3 coord;
    coord.y = iidx / NUM_BRICKS_X;
    coord.x = iidx - coord.y * NUM_BRICKS_X;
    coord.z = coord.y / NUM_BRICKS_Y;
    coord.y -= coord.z * NUM_BRICKS_Y;

    coord = 3 * coord + ivec3(1);
    return coord;
}

layout(binding = 0, rgba16f) uniform restrict image3D octreeBrickTex;

#endif // BRICK_TEX_SIZE

#endif // SHADERS_TREE_VOXEL_GLSL
