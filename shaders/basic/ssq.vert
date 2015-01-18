#version 440

out vec2 vsTexCoord;

void main()
{
    vsTexCoord = vec2((gl_VertexID == 0 || gl_VertexID == 2) ? 0.0 : 2.0,
                      (gl_VertexID == 2) ? 2.0 : 0.0);
    gl_Position = vec4((gl_VertexID == 0 || gl_VertexID == 2) ? -1.0 : 3.0,
                       (gl_VertexID == 2) ? 3.0 : -1.0,
                       -1.0,
                       1.0);
}
