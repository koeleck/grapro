#version 440 core

layout(location = 0) in vec2 Position;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec4 Colour;

out vec2 Frag_UV;
out vec4 Frag_Colour;

uniform mat4 ortho;

void main()
{
   Frag_UV = UV;
   Frag_Colour = Colour;
   gl_Position = ortho*vec4(Position.xy,0,1);
}
