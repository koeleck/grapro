#ifndef SHADERS_COMMON_CAMERA_GLSL
#define SHADERS_COMMON_CAMERA_GLSL

#include "bindings.glsl"

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!                                                 !!
//!!  keep in sync with src/core/shader_interface.h  !!
//!!                                                 !!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

struct Camera
{
    mat4        ViewMatrix;
    mat4        ProjMatrix;
    mat4        ProjViewMatrix;
    vec4        CameraPosition;
};

layout(std430, binding = CAMERA_BINDING) restrict readonly buffer CameraBlock
{
    Camera  cam;
};

#endif // SHADERS_COMMON_CAMERA_GLSL
