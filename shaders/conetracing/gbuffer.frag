#version 440

layout(location = 0) out vec4 out_FragColor;

uniform vec4 u_color;

void main()
{
    //out_FragColor = u_color;
    out_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
