#version 440 core

layout(location = 0) out vec4 outFragColor;

layout(binding = 0) uniform sampler2D uTexture;

void main()
{
    vec4 col = texelFetch(uTexture, ivec2(gl_FragCoord.xy), 0);

    outFragColor = vec4(pow(col.rgb, vec3(1.0 / 2.2)), col.a);
}
