#version 440

out vec2 vs_uv;

void main()
{
    gl_Position = vec4(gl_VertexID == 1 ? 3.0 : -1.0,
                       gl_VertexID == 2 ? 3.0 : -1.0,
                       0.0, 1.0);
    vs_uv = vec2(gl_VertexID == 1 ? 2.0 : 0.0,
                 gl_VertexID == 2 ? 2.0 : 0.0);
}
