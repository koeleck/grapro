#version 440

layout(location = 0) out vec4 out_FragColor;

layout(binding = 0) uniform sampler2D uTexture;

in vec2 vs_uv;
layout(location = 0) uniform int uLevel;

void main()
{
    ivec2 pos = ivec2(vs_uv * vec2(textureSize(uTexture, uLevel)));;
    //out_FragColor = texelFetch(uTexture, pos, uLevel);
    float val = texelFetch(uTexture, pos, uLevel).r;
    float zn = 1.0;
    float zf = 5000.0;
    float lin_dist = - (zn*zf) / (val * (zf - zn) - zf);
    out_FragColor = vec4(vec3((lin_dist - zn) / (zf - zn)), 1.0);
}
