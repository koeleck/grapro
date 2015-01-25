#version 440 core

#include "common/extensions.glsl"
#include "common/bindings.glsl"
#include "common/voxel.glsl"

layout(location = 0) out vec4 out_Color;

in vec4 worldPosition;

uniform vec3 u_bboxMin;
uniform vec3 u_bboxMax;
uniform uint u_voxelDim;
uniform uint u_maxLevel;

void main()
{
    const vec3 voxelSize = (u_bboxMax - u_bboxMin) / float(u_voxelDim);
    const ivec3 pos = ivec3(vec3(worldPosition.xyz - u_bboxMin) / voxelSize);
    uint childIdx = 0;
    uint nodePtr = octree[childIdx].id;
    int voxelDim = int(u_voxelDim);
    ivec3 umin = ivec3(0);

    // iterate through all tree levels
    for (uint i = 0; i < u_maxLevel - 1; ++i) {

        iterateTreeLevel(pos, nodePtr, voxelDim, childIdx, umin);

    }

    const vec4 col = octreeColor[childIdx].color;
    const vec4 emissive = octreeColor[childIdx].emissive;
    out_Color = vec4(col.xyz / col.w, 1.0) + vec4(emissive.xyz / emissive.w, 1.0);
}
