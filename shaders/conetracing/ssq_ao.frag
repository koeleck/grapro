#version 440 core

layout(location = 0) in vec4 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;
layout(location = 3) in float in_depth;

layout(location = 0) out vec4 out_color;

void main()
{
   out_color = vec4(1, 0, 0, 1);
}
