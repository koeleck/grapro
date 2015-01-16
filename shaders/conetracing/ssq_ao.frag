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

/******************************************************************************/

uniform sampler2D u_pos;
uniform sampler2D u_normal;
uniform sampler2D u_color;
uniform sampler2D u_depth;

layout(location = 0) out vec4 out_color;

const float M_PI          = 3.14159265359;
const uint num_sqrt_cones = 2; // 2x2 grid        

/******************************************************************************/

struct Cone
{
    vec3 pos;
    vec3 dir;
    float angle;
} cone[num_sqrt_cones*num_sqrt_cones];


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
        uy = ( 0.5 * step ) + y * step;

        for(uint x = 0; x < num_sqrt_cones; ++x)
        {
            ux = ( 0.5 * step ) + x * step;

            ONB onb = toONB(normal);
            vec3 v = UniformHemisphereSampling(ux, uy);
            cone[y * num_sqrt_cones + x].dir   = toWorld(onb, v);
            cone[y * num_sqrt_cones + x].pos   = pos.xyz;
            cone[y * num_sqrt_cones + x].angle = 180 / num_sqrt_cones;
        }
    }

    out_color = vec4(1, normal.x, normal.y, 1);
}
