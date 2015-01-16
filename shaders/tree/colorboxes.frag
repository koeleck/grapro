#version 440 core

#include "common/extensions.glsl"
#include "common/bindings.glsl"
#include "common/voxel.glsl"

layout(location = 0) out vec4 out_Color;

in VertexData
{
    vec3 viewdir;
    vec3 normal;
    vec2 uv;
    vec3 tangent;
    vec3 bitangent;
    flat uint materialID;
} inData;

in vec4 worldPosition;

uniform vec3 u_bboxMin;
uniform vec3 u_bboxMax;
uniform uint u_voxelDim;
uniform uint u_treeLevels;

struct octreeColorBuffer
{
    vec4    color;
};

layout(std430, binding = OCTREE_COLOR_BINDING) restrict buffer octreeColorBlock
{
    octreeColorBuffer octreeColor[];
};

void main()
{

	vec3 voxelSize = (u_bboxMax - u_bboxMin) / float(u_voxelDim);
	ivec3 pos = ivec3(vec3(worldPosition.xyz - u_bboxMin) / voxelSize);
	vec3 clamped = vec3(pos) / float(u_voxelDim);

	// local vars
    uint childIdx = 0;
    uint nodePtr = octree[childIdx].id;

	uint voxelDim = u_voxelDim;

    uvec3 umin = uvec3(0);

	// iterate through all tree levels
    for (uint i = 0; i < u_treeLevels - 1; ++i) {

        // go to next dimension
        voxelDim /= 2;

        // mask out flag bit to get child idx
        childIdx = int(nodePtr & 0x7FFFFFFF);

        // create subnodes
        ivec3 subnodeptrXYZ = clamp(ivec3(1 + pos - umin - voxelDim), 0, 1);

        int subnodeptr = subnodeptrXYZ.x;
        subnodeptr += 2 * subnodeptrXYZ.y;
        subnodeptr += 4 * subnodeptrXYZ.z;

        childIdx += subnodeptr;

        umin.x += voxelDim * subnodeptrXYZ.x;
        umin.y += voxelDim * subnodeptrXYZ.y;
        umin.z += voxelDim * subnodeptrXYZ.z;

        // update node
        nodePtr = octree[childIdx].id;
    }

    vec4 col = octreeColor[childIdx].color;

	out_Color = vec4(col.xyz / col.w, 1.0);
	//out_Color = vec4(clamped, 1.0);
	out_Color = vec4(octree[0].id) / 2;
}
