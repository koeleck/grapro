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
    mat4        ViewMatrix_T;
    mat4        ProjMatrix_T;
    mat4        ProjViewMatrix_T;
    mat4        InvProjViewMatrix_T;
    vec4        Position;
    vec4        padding[3];
};

layout(std430, binding = CAMERA_BINDING) restrict readonly buffer CameraBlock
{
    Camera  cam;
};

#endif // SHADERS_COMMON_CAMERA_GLSL
