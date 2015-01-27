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
    uint x_negative;

    vec3 diffuse;
    uint x_positive;

    vec3 emissive;
    uint y_negative;

    vec3 normal;
    uint y_positive;

    uint z_negative;
    uint z_positive;
    uint count;
    uint padding;
};

layout(std430, binding = OCTREE_INFO_BINDING) restrict buffer OctreeInfoBlock
{
    NodeInfo octreeInfo[];
};

#if defined(NUM_BRICKS_X) && defined(NUM_BRICKS_Y) && defined(NUM_BRICKS_Z)

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

vec3 getBrickTexCoord(in uint idx, vec3 posInVoxel)
{
    posInVoxel = clamp(posInVoxel, vec3(0.0), vec3(1.0));
    vec3 off = 2.0 * posInVoxel - 0.5;
    return (vec3(getBrickCoord(idx)) + off) / (3.0 * vec3(NUM_BRICKS_X, NUM_BRICKS_Y, NUM_BRICKS_Z));
}

layout(binding = 0, rgba16f) uniform restrict image3D octreeBrickTex;

layout(binding = BRICK_TEX) uniform sampler3D uOctree3DTex;

vec4 sampleBrick(in ivec3 brickCoord, in ivec3 center, in bool fullRange)
{
    ivec3 minP = max(center - ivec3(1), ivec3(-1));
    ivec3 maxP = min(center + ivec3(1), ivec3(1)) + (fullRange ? ivec3(1) : ivec3(0));

    float weights[4] = float[4](1.0 /  8.0,
                                1.0 / 16.0,
                                1.0 / 32.0,
                                1.0 / 64.0);

    vec4 result = vec4(0.0);
    for (int z = minP.z; z < maxP.z; ++z) {
        for (int y = minP.y; y < maxP.y; ++y) {
            for (int x = minP.x; x < maxP.x; ++x) {
                ivec3 pos = ivec3(x, y, z);
                ivec3 diff = abs(pos - center);
                float w = weights[diff.x + diff.y + diff.z];
                result += w * imageLoad(octreeBrickTex, brickCoord + pos);
            }
        }
    }
    return result;
}

#endif // NUM_BRICKS_*

#endif // SHADERS_TREE_VOXEL_GLSL
