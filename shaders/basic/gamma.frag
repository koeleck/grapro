#version 440 core

layout(location = 0) out vec4 outFragColor;

layout(binding = 0) uniform sampler2D uTexture;

layout(location = 0) uniform float uGamma;

void main()
{
    vec4 col = texelFetch(uTexture, ivec2(gl_FragCoord.xy), 0);

    outFragColor = vec4(pow(col.rgb, vec3(uGamma)), col.a);
}
