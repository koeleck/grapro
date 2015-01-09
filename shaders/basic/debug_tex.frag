#version 440

layout(location = 0) out vec4 out_FragColor;

layout(binding = 0) uniform sampler2D uTexture;

in vec2 vs_uv;
layout(location = 0) uniform int uLevel;

void main()
{
    ivec2 pos = ivec2(vs_uv * vec2(textureSize(uTexture, 0)));;
    pos >>= uLevel;
    out_FragColor = texelFetch(uTexture, pos, uLevel);
    //out_FragColor = texture(uTexture, vs_uv);
}
