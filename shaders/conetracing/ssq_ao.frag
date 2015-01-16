#version 440 core

uniform sampler2D u_pos;
uniform sampler2D u_normal;
uniform sampler2D u_color;
uniform sampler2D u_depth;

layout(location = 0) out vec4 out_color;

void main()
{
   vec4 pos    = texture(u_pos, gl_FragCoord.xy);
   vec3 normal = texture(u_normal, gl_FragCoord.xy).xyz;
   float depth = texture(u_color, gl_FragCoord.xy).x;
   vec3 color  = texture(u_depth, gl_FragCoord.xy).xyz;

   out_color = vec4(1, pos.x, pos.y, 1);
}
