#version 440 core

#include "common/extensions.glsl"
#include "common/voxel.glsl"

/******************************************************************************/

uniform uint u_voxelDim;
uniform vec3 u_bboxMin;
uniform vec3 u_bboxMax;
uniform uint u_screenwidth;
uniform uint u_screenheight;
uniform uint u_maxlevel;

/******************************************************************************/

uniform sampler2D u_pos;
uniform sampler2D u_normal;
uniform sampler2D u_color;
uniform sampler2D u_depth;

layout(location = 0) out vec4 out_color;

const uint num_sqrt_cones = 2;   // 2x2 grid
const uint max_samples    = 3; // how many samples to take along each cone?
#define M_PI 3.1415926535897932384626433832795

struct Cone
{
    vec3 pos;
    vec3 dir;
    float angle;
} cone[num_sqrt_cones * num_sqrt_cones];

/******************************************************************************/

float ConeRadiusAtDistance(uint idx, float distance)
{
    const float angle  = cone[idx].angle * 0.5;
    return distance * tan(angle);
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

vec4 getColor(uint maxlevel, vec3 wpos)
{
    vec3 voxelSize = (u_bboxMax - u_bboxMin) / float(u_voxelDim);
    ivec3 pos = ivec3(vec3(wpos - u_bboxMin) / voxelSize);

    // local vars
    uint childIdx = 0;
    uint nodePtr = octree[childIdx].id;

    uint voxelDim = u_voxelDim;

    uvec3 umin = uvec3(0);

    // iterate through all tree levels
    for (uint i = 0; i < maxlevel; ++i) {

        if ((nodePtr & 0x80000000) == 0) {
        	// no flag set -> no child node
        	return vec4(0);
        }

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
    col /= col.w;
    return col;

}

/******************************************************************************/

void main()
{

    vec2 uv = gl_FragCoord.xy / vec2(u_screenwidth, u_screenheight);

    vec4 pos    = texture(u_pos, uv);
    vec3 normal = texture(u_normal, uv).xyz;
    float depth = texture(u_depth, uv).x;
    vec3 color  = texture(u_color, uv).xyz;

    // AO: count number of occluded cones
    uint occluded_cone = 0;

    // cone tracing
    const float step = (1.f / float(num_sqrt_cones));
    float ux = 0.f;
    float uy = 0.f;

    vec4 totalColor = vec4(0);

    for(uint y = 0; y < num_sqrt_cones; ++y) {

        uy = (0.5f + y) * step;

        for(uint x = 0; x < num_sqrt_cones; ++x) {

            ux = (0.5f + x) * step;
            const uint idx = y * num_sqrt_cones + x;

            // create the cone
            ONB onb = toONB(normal);
            vec3 v = UniformHemisphereSampling(ux, uy);
            cone[idx].dir   = toWorld(onb, v);
            cone[idx].pos   = pos.xyz;
            cone[idx].angle = 180 / num_sqrt_cones;

            // trace the cone for each sample
            for(uint s = 0; s < max_samples; ++s) {

                const float d = 5; // has to be smaller than the current voxel size
                const float sample_distance = s * d;
                const float diameter = 2 * ConeRadiusAtDistance(idx, sample_distance);

                // find the corresponding mipmap level.
                // start at 0: the final level that is found will be 1 level too much,
                // because we need the first voxel where the diameter fits and not the first
                // voxel where the diameter does not fit anymore
                uint level = 0;
                vec3 voxel_size = (u_bboxMax - u_bboxMin) / float(u_voxelDim);
                while(voxel_size.x > diameter) {
                    voxel_size /= 2.f;
                    ++level;
                }

                // ambient occlusion
                vec3 wpos = pos.xyz + sample_distance * cone[idx].dir;
                vec4 color = getColor(level, wpos);
                totalColor += color;

            }
        }
    }

    if (totalColor.w > 0) {
    	totalColor /= totalColor.w;
    }
    out_color = totalColor;

    // debug
    //out_color = vec4(normal, 1);
}
