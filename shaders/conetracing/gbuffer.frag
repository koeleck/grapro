#version 440

layout(location = 0) out vec4 out_FragColor;

in vec4 worldPosition;

uniform vec4 u_color;

void main()
{
    out_FragColor = worldPosition;
}
