#ifndef SHADERS_COMMON_LIGHTS_GLSL
#define SHADERS_COMMON_LIGHTS_GLSL

#include "bindings.glsl"

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!                                                 !!
//!!  keep in sync with src/core/shader_interface.h  !!
//!!                                                 !!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

struct Light
{
    mat4    projViewMatrix;

    vec3    position;
    float   angleInnerCone;

    vec3    direction;
    float   angleOuterCone;

    vec3    intensity;
    float   maxDistance;

    float   constantAttenuation;
    float   linearAttenuation;
    float   quadraticAttenuation;
    int     type_texid; // 31:      is shadowcasting
                        // [30:29]: type
                        // [28:0]:  depth texture index
};

layout(std430, binding = LIGHT_BINDING) restrict readonly buffer LightBlock
{
    int         numLights;
    Light       lights[];
};

#endif // SHADERS_COMMON_LIGHTS_GLSL
