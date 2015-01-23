#version 440 core

#include "common/extensions.glsl"
#include "common/voxel.glsl"

layout(location = 0) out vec4 out_color;

/******************************************************************************/

uniform uint u_voxelDim;
uniform vec3 u_bboxMin;
uniform vec3 u_bboxMax;
uniform uint u_screenwidth;
uniform uint u_screenheight;
uniform uint u_treeLevels;
uniform uint u_coneGridSize;
uniform uint u_numSteps;

uniform sampler2D u_pos;
uniform sampler2D u_normal;

/******************************************************************************/

const float voxelSize = (u_bboxMax.x - u_bboxMin.x) / float(u_voxelDim);

/******************************************************************************/

vec4 getColor(uint maxlevel, vec3 wpos)
{
    const ivec3 pos = ivec3((wpos - u_bboxMin) / voxelSize);
    uint childIdx = 0;
    uint nodePtr = octree[childIdx].id;
    int voxelDim = int(u_voxelDim);
    ivec3 umin = ivec3(0);

    for (uint i = 0; i < maxlevel - 1; ++i) {

        if ((nodePtr & 0x80000000) == 0) {
            // no flag set -> no child nodes
            return vec4(0);
        }

        iterateTreeLevel(pos, nodePtr, voxelDim, childIdx, umin);

    }

    vec4 col = octreeColor[childIdx].color;
    if (col.w == 0.f) return vec4(0);
    col /= col.w;
    return col;
}

/******************************************************************************/

vec4 getNormal(uint maxlevel, vec3 wpos)
{
    const ivec3 pos = ivec3((wpos - u_bboxMin) / voxelSize);
    uint childIdx = 0;
    uint nodePtr = octree[childIdx].id;
    int voxelDim = int(u_voxelDim);
    ivec3 umin = ivec3(0);

    for (uint i = 0; i < maxlevel - 1; ++i) {

        if ((nodePtr & 0x80000000) == 0) {
            // no flag set -> no child nodes
            return vec4(0);
        }

        iterateTreeLevel(pos, nodePtr, voxelDim, childIdx, umin);

    }

    vec4 normal = octreeColor[childIdx].normal;
    if (normal.w == 0.f) return vec4(0);
    normal /= normal.w;
    return normal;
}

/******************************************************************************/

void main()
{

    const vec2 uv = gl_FragCoord.xy / vec2(u_screenwidth, u_screenheight);
    const vec3 normal = texture(u_normal, uv).xyz;
    if (normal == 0) return;
    const vec3 pos = texture(u_pos, uv).xyz;

    const float step = (1.f / float(u_coneGridSize));
    vec4 totalColor = vec4(0);

    for (uint y = 0; y < u_coneGridSize; ++y) {

        float uy = (0.5f + float(y)) * step;

        for (uint x = 0; x < u_coneGridSize; ++x) {

            float ux = (0.5f + float(x)) * step;

            // create the cone
            ONB onb = toONB(normal);
            vec3 v = uniformHemisphereSampling(ux, uy);
            Cone cone;
            cone.dir   = normalize(toWorld(onb, v));
            cone.angle = 180.f / float(u_coneGridSize);

            // calculate weight
            float d = abs(dot(normalize(normal), cone.dir));

            // trace the cone for each sample
            for (uint step = 1; step <= u_numSteps; ++step) {

                const float totalDist = step * voxelSize;
                const float diameter = 2 * coneRadiusAtDistance(cone, totalDist);
                if (diameter <= 0.f) continue; // some error (angle < 0 || angle > 90)

                // calculate mipmap level
                int level = int(u_treeLevels);
                float voxel_size = voxelSize;
                while (voxel_size < diameter && level > 0) {
                    voxel_size *= 2;
                    --level;
                }

                // get indirect color
                const vec3 wpos = pos + totalDist * cone.dir;
                const vec3 normal = getNormal(level, wpos).xyz;
                /*if (dot(normal, cone.dir) > 0.f) {
                    continue;
                }*/
                vec4 color = getColor(level, wpos);
                if (color.w > 0) {
                    // color found -> stop walking!
                    color.xyz *= d*d;
                    color /= (step * step * voxelSize * voxelSize);
                    totalColor += color;
                    break;
                }

            }

        }

    }

    if (totalColor.w > 0) {
        totalColor /= totalColor.w;
    }

    out_color = totalColor;

}

/******************************************************************************/
