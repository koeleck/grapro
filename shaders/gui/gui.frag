#version 440 core

layout(location = 0) out vec4 FragColor;

in vec2 Frag_UV;
in vec4 Frag_Colour;

uniform sampler2D Texture;

void main()
{
   FragColor = Frag_Colour * texture(Texture, Frag_UV.st);
}

