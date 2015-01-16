#version 440 core

layout(location = 0) in vec4 ssq_pos;

void main()
{
   gl_Position = ssq_pos;
}
