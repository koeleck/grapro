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
    mat4    ProjViewMatrix;

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

#define LIGHT_TYPE_SPOT         0x10000000
#define LIGHT_TYPE_DIRECTIONAL  0x20000000
#define LIGHT_TYPE_POINT        0x40000000
#define LIGHT_TEXID_BITS        0x0fffffff

layout(std430, binding = LIGHT_BINDING) restrict readonly buffer LightBlock
{
    int         numLights;
    Light       lights[];
};

#endif // SHADERS_COMMON_LIGHTS_GLSL
