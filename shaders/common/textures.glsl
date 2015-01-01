#ifndef SHADERS_COMMON_TEXTURES_GLSL
#define SHADERS_COMMON_TEXTURES_GLSL

#include "bindings.glsl"

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!                                                 !!
//!!  keep in sync with src/core/shader_interface.h  !!
//!!                                                 !!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

struct Texture
{
    uvec2   handle;
    int     num_channels;
    int     _padding;
};

layout(std430, binding = TEXTURE_BINDING) restrict readonly buffer TextureBlock
{
    Texture     textures[];
};



#endif // SHADERS_COMMON_TEXTURES_GLSL
