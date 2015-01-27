#version 440

layout(location = 0) out vec4 outFragColor;

in vec2 vsTexCoord;

layout(binding = 0) uniform sampler2D uTexture;

void main()
{
    outFragColor = texture(uTexture, vsTexCoord);
}
