#ifndef SHADERS_TREE_VOXEL_GLSL
#define SHADERS_TREE_VOXEL_GLSL

#include "common/bindings.glsl"

struct VoxelFragment
{
    uint position_diff_r;
    uint diff_gb_normal_xy;
    uint normal_z_emissive;
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
    uint x_neg_neighbor;
    uint x_pos_neighbor;
    uint y_neg_neighbor;
    uint y_pos_neighbor;
    uint z_neg_neighbor;
    uint z_pos_neighbor;
    uint padding[10];
}; // 64 bytes

struct LeafInfo
{
    vec4 diffuse;
    vec4 emissive;
    vec4 normal;
    vec3 position;
    uint count;
}; // 64 bytes

layout(std430, binding = OCTREE_INFO_BINDING) restrict buffer OctreeInfoBlock
{
    NodeInfo octreeInfo[];
};

layout(std430, binding = OCTREE_COLOR_BINDING) restrict buffer OctreeColorBlock
{
    LeafInfo octreeColor[];
};

#endif // SHADERS_TREE_VOXEL_GLSL
