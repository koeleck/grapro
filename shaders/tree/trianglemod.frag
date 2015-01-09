#version 440 core

layout(location = 0) out vec4 out_Color;

flat in vec3 dominantAxis;
flat in vec4 f_AABB;

void main()
{
    out_Color = vec4(dominantAxis, 0.f);
}
