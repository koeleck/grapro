#ifndef SHADER_COMMON_VOXEL_GLSL
#define SHADER_COMMON_VOXEL_GLSL

#include "bindings.glsl"

/******************************************************************************/

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
    vec4    color;
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

#endif // SHADER_COMMON_VOXEL_GLSL
