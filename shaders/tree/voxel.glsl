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


// these two structs must have the same size, because we
// store them in the same buffer
struct NodeInfo
{
    vec3 position;
    uint x_neg_neighbor;
    uint x_pos_neighbor;
    uint y_neg_neighbor;
    uint y_pos_neighbor;
    uint z_neg_neighbor;
    uint z_pos_neighbor;
    uint padding[7];
}; // 64 bytes

struct LeafInfo
{
    vec3 position;
    uint count;
    vec4 diffuse;
    vec4 emissive;
    vec4 normal;
}; // 64 bytes

layout(std430, binding = OCTREE_INFO_BINDING) restrict buffer OctreeInfoBlock
{
    NodeInfo octreeInfo[];
};

layout(std430, binding = OCTREE_COLOR_BINDING) restrict buffer OctreeColorBlock
{
    LeafInfo octreeColor[];
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
