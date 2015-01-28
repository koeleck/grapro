#ifndef SHADERS_SMAA_SMAA_GLSL
#define SHADERS_SMAA_SMAA_GLSL

layout(location = 0) uniform vec2 uViewportSize;

#define SMAA_GLSL_4 1
#define SMAA_RT_METRICS vec4(1.0 / uViewportSize.x, 1.0 / uViewportSize.y, uViewportSize.x, uViewportSize.y)

//#define SMAA_PRESET_LOW 1
//#define SMAA_PRESET_MEDIUM 1
//#define SMAA_PRESET_HIGH 1
#define SMAA_PRESET_ULTRA 1


#if defined(VERTEX_SHADER)
#define SMAA_INCLUDE_VS 1
#elif defined(FRAGMENT_SHADER)
#define SMAA_INCLUDE_PS 1
#endif

#include "SMAA.hlsl"

#endif // SHADERS_SMAA_SMAA_GLSL
