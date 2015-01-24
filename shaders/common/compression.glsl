#ifndef SHADERS_COMMON_COMPRESSION_GLSL
#define SHADERS_COMMON_COMPRESSION_GLSL

uint packUNorm11_11_10(in vec3 value)
{
    uint3 tmp = uint3(uint(value.x * 2047.0),
                      uint(value.y * 2047.0),
                      uint(value.z * 1023.0));
    return ((tmp.x & 0x000007FF) << 21u) | ((tmp.y & 0x000007FF) << 10u) | (tmp.z & 0x000003FF);
}

vec3 unpackUNorm11_11_10(in uint value)
{
    vec3 tmp = vec3(float((col >> 21u) & 0x000007FF) / 2047.f,
                    float((col >> 10u) & 0x000007FF) / 2047.f,
                    float(col & 0x000003FF) / 1023.f);
    return tmp;
}

#endif // SHADERS_COMMON_COMPRESSION_GLSL
