#ifndef SHADERS_COMMON_CAMERA_GLSL
#define SHADERS_COMMON_CAMERA_GLSL

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!  keep in sync with 3d/camera.cpp !!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// TODO assign binding index somewhere else
layout(std140, binding = 0) uniform CameraBlock {
    mat4        ViewMatrix;
    mat4        ProjMatrix;
    mat4        ProjViewMatrix;
    vec4        CameraPosition;
};

#endif // SHADERS_COMMON_CAMERA_GLSL
