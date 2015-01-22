#version 440

out vec2 vsTexCoord;

void main()
{
    vsTexCoord = vec2(float(gl_VertexID & 0x01) * 2.0,
                      float(gl_VertexID & 0x02));
    gl_Position = vec4(fma(vsTexCoord, vec2(2.0), vec2(-1.0)),
                       -1.0,
                       1.0);
}
