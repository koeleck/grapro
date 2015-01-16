#version 440 core

#include "common/voxel.glsl"

/******************************************************************************/

struct octreeColorBuffer
{
    vec4    color;
};

layout(std430, binding = OCTREE_COLOR_BINDING) restrict buffer octreeColorBlock
{
    octreeColorBuffer octreeColor[];
};

uniform uint u_voxelDim;

/******************************************************************************/

uniform sampler2D u_pos;
uniform sampler2D u_normal;
uniform sampler2D u_color;
uniform sampler2D u_depth;

layout(location = 0) out vec4 out_color;

const float M_PI          = 3.14159265359;
const uint num_sqrt_cones = 2;   // 2x2 grid 
const uint max_samples    = 100; // how many samples to take along each cone?

/******************************************************************************/

struct Cone
{
    vec3 pos;
    vec3 dir;
    float angle;
} cone[num_sqrt_cones * num_sqrt_cones];

float ConeAreaAtDistance(uint idx, float distance)
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
    const float angle  = cone[idx].angle * 0.5 + 90;
    const float radius = distance / tan(angle);
    return 2 * M_PI * radius;
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

void main()
{
    vec4 pos    = texture(u_pos, gl_FragCoord.xy);
    vec3 normal = texture(u_normal, gl_FragCoord.xy).xyz;
    float depth = texture(u_color, gl_FragCoord.xy).x;
    vec3 color  = texture(u_depth, gl_FragCoord.xy).xyz;

    // cone tracing
    const float step = (1.f / float(num_sqrt_cones));
    float ux = 0.f;
    float uy = 0.f;

    for(uint y = 0; y < num_sqrt_cones; ++y)
    {
        uy = (0.5 * step) + y * step;

        for(uint x = 0; x < num_sqrt_cones; ++x)
        {
            ux = (0.5 * step) + x * step;

            // create the cone
            ONB onb = toONB(normal);
            vec3 v = UniformHemisphereSampling(ux, uy);
            cone[y * num_sqrt_cones + x].dir   = toWorld(onb, v);
            cone[y * num_sqrt_cones + x].pos   = pos.xyz;
            cone[y * num_sqrt_cones + x].angle = 180 / num_sqrt_cones;

            // trace the cone for each sample
            for(uint s = 0; s < max_samples; ++s)
            {
                const float d = 17; // has to be smaller than the current voxel size
                const float sample_distance = s * d;
                const float area = ConeAreaAtDistance(y * num_sqrt_cones + x, sample_distance);

                // find the corresponding mipmap level.
                // start at -1: the final level that is found will be 1 level to much,
                // because we need the first voxel where the area fits and not the first
                // voxel where the area does not fit anymore
                uint level = -1;
                float voxel_size = u_voxelDim;
                while(voxel_size > area)
                {
                    voxel_size = u_voxelDim / 2;
                    ++level;
                }
            }
        }
    }

    out_color = vec4(1, normal.x, normal.y, 1);
}
