#version 440 core

layout(location = 0) in vec4 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;
layout(location = 3) in float in_depth;

layout(location = 4) in vec4 ssq_pos;

void main()
{
   gl_Position = ssq_pos;
}
