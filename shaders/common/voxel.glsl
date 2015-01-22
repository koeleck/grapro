#ifndef SHADER_COMMON_VOXEL_GLSL
#define SHADER_COMMON_VOXEL_GLSL

#include "bindings.glsl"

/******************************************************************************/

struct voxelStruct {
    uint position; // 2 bits unused, 10/10/10 bits pos.x/y/z
    uint color; // 11/11/10 RGB
    uint padding, padding2;
};

layout (std430, binding = VOXEL_BINDING) restrict buffer voxelBlock {
    voxelStruct voxel[];
};

struct octreeBuffer
{
    uint    id;
};

layout(std430, binding = OCTREE_BINDING) restrict buffer octreeBlock
{
    octreeBuffer octree[];
};

struct octreeColorBuffer
{
    vec4 color;
};

layout(std430, binding = OCTREE_COLOR_BINDING) restrict buffer octreeColorBlock
{
    octreeColorBuffer octreeColor[];
};

/******************************************************************************/

#define M_PI 3.1415926535897932384626433832795

struct Cone
{
    vec3 dir;
    float angle; // in degrees
};

struct ONB
{
    vec3 N, S, T;
};

/******************************************************************************/

float degreesToRadians(float deg)
{
    return deg * M_PI / 180.f;
}

/******************************************************************************/

float radiansToDegrees(float rad)
{
    return rad * 180.f / M_PI;
}

/******************************************************************************/

float coneRadiusAtDistance(Cone cone, float distance)
{
    const float angle  = cone.angle * 0.5f;
    return distance * tan(degreesToRadians(angle));
}

/******************************************************************************/

vec3 uniformHemisphereSampling(float ux, float uy)
{
    clamp(ux, 0.f, 1.f);
    clamp(uy, 0.f, 1.f);
    const float r = sqrt(1.f - ux * ux);
    const float phi = 2 * M_PI * uy;
    return vec3(cos(phi) * r, sin(phi) * r, ux);
}

/******************************************************************************/

ONB toONB(vec3 normal)
{
    ONB onb;
    onb.N = normal;
    if(abs(normal.x) > abs(normal.z)) {
        onb.S.x = -normal.y;
        onb.S.y = normal.x;
        onb.S.z = 0;
    } else {
        onb.S.x = 0;
        onb.S.y = -normal.z;
        onb.S.z = normal.y;
    }

    onb.S = normalize(onb.S);
    onb.T = cross(onb.S, onb.N);
    return onb;
}

/******************************************************************************/

vec3 toWorld(ONB onb, vec3 v)
{
    return onb.S * v.x + onb.T * v.y + onb.N * v.z;
}

/******************************************************************************/

uvec3 convertPosition(in uint pos)
{
    return uvec3((pos >> 20u) & 0x000003FF, (pos >> 10u) & 0x000003FF, pos & 0x000003FF);
}

/******************************************************************************/

uint convertPosition(in uvec3 pos)
{
    return ((pos.x & 0x000003FF) << 20u) | ((pos.y & 0x000003FF) << 10u) | (pos.z & 0x000003FF);
}

/******************************************************************************/

vec3 convertColor(in uint col)
{
    float r = float((col >> 21u) & 0x000007FF) / 2047.f;
    float g = float((col >> 10u) & 0x000007FF) / 2047.f;
    float b = float(col & 0x000003FF) / 1023.f;
    return vec3(r,g,b);
}

/******************************************************************************/

uint convertColor(in vec3 col)
{
    uint r = uint(col.r * 2047.f);
    uint g = uint(col.g * 2047.f);
    uint b = uint(col.b * 1023.f);
    return ((r & 0x000007FF) << 21u) | ((g & 0x000007FF) << 10u) | (b & 0x000003FF);
}

/******************************************************************************/

#endif // SHADER_COMMON_VOXEL_GLSL
