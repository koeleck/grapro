#ifndef SHADERS_COMMON_COMPRESSION_GLSL
#define SHADERS_COMMON_COMPRESSION_GLSL

uint packUNorm11_11_10(in vec3 value)
{
    uvec3 tmp = uvec3(uint(value.x * 2047.0),
                      uint(value.y * 2047.0),
                      uint(value.z * 1023.0));
    return ((tmp.x & 0x000007FF) << 21u) | ((tmp.y & 0x000007FF) << 10u) | (tmp.z & 0x000003FF);
}

vec3 unpackUNorm11_11_10(in uint value)
{
    vec3 tmp = vec3(float((value >> 21u) & 0x000007FF) / 2047.f,
                    float((value >> 10u) & 0x000007FF) / 2047.f,
                    float(value & 0x000003FF) / 1023.f);
    return tmp;
}

uint packUInt2x16(in uvec2 value)
{
    return (value.x << 16) | (value.y & 0x0000FFFF);
}

uvec2 unpackUInt2x16(in uint value)
{
    return uvec2(value >> 16, value & 0x0000FFFF);
}

uint packUInt3x10(in uvec3 value)
{
    return ((value.x & 0x000003FF) << 20u) | ((value.y & 0x000003FF) << 10u) | (value.z & 0x000003FF);
}

uvec3 unpackUInt3x10(in uint value)
{
    return uvec3((value >> 20u) & 0x000003FF, (value >> 10u) & 0x000003FF, value & 0x000003FF);
}

vec3 RGB2YCoCg(in vec3 color)
{
    /* GPU Pro 4 "Pracical Framebuffer Compression"
        [Y ]   [1/4   1/2   1/4] [R]
        [Co] = [1/2    0   -1/2] [G]
        [Cg]   [-1/4  1/2  -1/4] [B]
    */
    return vec3(dot(vec3(0.25, 0.5,  0.25), color),
                dot(vec3(0.5,   0 , -0.5), color),
                dot(vec3(-0.25, 0.5, -0.25), color));
}

vec3 YCoCg2RGB(in vec3 color)
{
    /* GPU Pro 4 "Pracical Framebuffer Compression"
       R = Y + Co - Cg
       G = Y + Cg
       B = Y - Co - Cg
    */
    return vec3(color.x + color.y - color.z,
                color.x +           color.z,
                color.x - color.y - color.z);
}

vec2 compressNormal(in vec3 normal)
{
    // http://en.wikipedia.org/wiki/Stereographic_projection
    float tmp = 1.0 - normal.z;
    return vec2(normal.x / tmp, normal.y / tmp);
}

vec3 decompressNormal(in vec2 normal)
{
    float tmp = 1.0 + normal.x*normal.x + normal.y*normal.y;
    return vec3(2.0 * normal.x / tmp,
                2.0 * normal.y / tmp,
                1.0 - 2.0 / tmp);
}


#endif // SHADERS_COMMON_COMPRESSION_GLSL
