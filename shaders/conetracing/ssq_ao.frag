#version 440 core

#include "common/voxel.glsl"

/******************************************************************************/

uniform uint u_voxelDim;
uniform vec3 u_bboxMin;
uniform vec3 u_bboxMax;
uniform uint u_screenwidth;
uniform uint u_screenheight;
uniform uint u_treelevel;

/******************************************************************************/

uniform sampler2D u_pos;
uniform sampler2D u_normal;
uniform sampler2D u_color;
uniform sampler2D u_depth;

layout(location = 0) out vec4 out_color;

uniform uint u_num_sqrt_cones;
uniform uint u_max_samples;
uniform uint u_sample_interval;

const float M_PI            = 3.14159265359;

/******************************************************************************/

const uint max_num_sqrt_cones = 4;
struct Cone
{
    vec3 pos;
    vec3 dir;
    float angle;
} cone[max_num_sqrt_cones * max_num_sqrt_cones];

float ConeDiameterAtDistance(uint idx, float distance)
{
    /*
        http://www.mathematische-basteleien.de/kegel.htm
                 ^
                /|\  <-- angle * 0.5 + 90
               / | \
              / d|  \
             /   |   \
            /____|____\ <-- angle * 0.5
                   r
    */
    const float angle  = cone[idx].angle * 0.5;
    const float radius = distance * tan(angle);
    return abs(2 * radius);
}

/******************************************************************************/

vec3 UniformHemisphereSampling(float ux, float uy)
{
    const float r = sqrt(1.f - ux * ux);
    const float phi = 2 * M_PI * uy;
    return vec3(cos(phi) * r, sin(phi) * r, ux);
}

/******************************************************************************/

struct ONB
{
    vec3 N, S, T;
};

ONB toONB(vec3 normal)
{
    ONB onb;
    onb.N = normal;
    if(abs(normal.x) > abs(normal.z))
    {
        onb.S.x = -normal.y;
        onb.S.y = normal.x;
        onb.S.z = 0;
    }
    else
    {
        onb.S.x = 0;
        onb.S.y = -normal.z;
        onb.S.z = normal.y;
    }

    onb.S = normalize(onb.S);
    onb.T = cross(onb.S, onb.N);
    return onb;
}

vec3 toWorld(ONB onb, vec3 v)
{
    return onb.S * v.x + onb.T * v.y + onb.N * v.z;
}

/******************************************************************************/

bool checkOcclusion(uint maxlevel, vec3 wpos)
{
    vec3 voxelSize = (u_bboxMax - u_bboxMin) / float(u_voxelDim);
    ivec3 pos = ivec3(vec3(wpos - u_bboxMin) / voxelSize);
    vec3 clamped = vec3(pos) / float(u_voxelDim);

    bool is_occluded = true;

    // local vars
    uint childIdx = 0;
    uint nodePtr = octree[childIdx].id;

    uint voxelDim = u_voxelDim;

    uvec3 umin = uvec3(0);

    if(maxlevel > u_treelevel)
        maxlevel = u_treelevel;

    // iterate through all tree levels
    for (uint i = 0; i < maxlevel; ++i) {

        // go to next dimension
        voxelDim /= 2;

        // check occlusion
        if((nodePtr & 0x80000000) == 0)
        {
            is_occluded = false;
            return is_occluded;
        }

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

    return is_occluded;
}

/******************************************************************************/

void main()
{
    vec2 tex_coord = gl_FragCoord.xy / vec2(u_screenwidth, u_screenheight);

    vec4 pos    = texture(u_pos, tex_coord);
    vec3 normal = texture(u_normal, tex_coord).xyz;
    float depth = texture(u_depth, tex_coord).x;
    vec3 color  = texture(u_color, tex_coord).xyz;

    // AO: count number of occluded cones
    uint occluded_cones = 0;

    // cone tracing
    const float step = (1.f / float(u_num_sqrt_cones));
    float ux = 0.f;
    float uy = 0.f;

    for(uint y = 0; y < u_num_sqrt_cones; ++y)
    {
        uy = (0.5 * step) + y * step;

        for(uint x = 0; x < u_num_sqrt_cones; ++x)
        {
            ux = (0.5 * step) + x * step;
            const uint idx = y * u_num_sqrt_cones + x;

            // create the cone
            ONB onb = toONB(normal);
            vec3 v = UniformHemisphereSampling(ux, uy);
            cone[idx].dir   = normalize(toWorld(onb, v));
            cone[idx].pos   = pos.xyz;
            cone[idx].angle = 180 / u_num_sqrt_cones;

            // trace the cone for each sample
            for(uint s = 0; s < u_max_samples; ++s)
            {
                const float sample_distance = s * u_sample_interval;
                const float diameter = ConeDiameterAtDistance(idx, sample_distance);

                // find the corresponding mipmap level.
                // start at -1: the final level that is found will be 1 level to much,
                // because we need the first voxel where the diameter fits and not the first
                // voxel where the diameter does not fit anymore
                uint level = 0;
                vec3 voxel_size_xyz = (u_bboxMax - u_bboxMin) / float(u_voxelDim);

                // find maximum of x/y/z dimension
                float voxel_size = max(voxel_size_xyz.x, voxel_size_xyz.y);
                voxel_size = max(voxel_size_xyz.y, voxel_size_xyz.z);
                voxel_size = max(voxel_size_xyz.x, voxel_size_xyz.z);

                while(voxel_size > diameter)
                {
                    voxel_size /= 2;
                    ++level;
                }

                // ambient occlusion
                vec3 wpos = pos.xyz + sample_distance * cone[idx].dir;
                if(checkOcclusion(level - 1, wpos))
                {
                    // we are occluded here
                    ++occluded_cones;
                }
            }
        }
    }

    // AO
    const float ratio = float(occluded_cones) / (float(u_max_samples) * float(u_num_sqrt_cones * u_num_sqrt_cones));

    out_color = vec4(color, 1);
    out_color *= ratio;
}
