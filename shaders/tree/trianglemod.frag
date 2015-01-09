#version 440 core

layout(location = 0) out vec4 out_Color;

flat in vec3 dominantAxis;
flat in int f_axis;
flat in vec4 f_AABB;

in vec3 f_pos;

uniform int u_width;
uniform int u_height;

void main()
{
    if(f_pos.x < f_AABB.x || f_pos.y < f_AABB.y || f_pos.x > f_AABB.z || f_pos.y > f_AABB.w) {
		discard;
    }

    uvec4 temp = uvec4(gl_FragCoord.xy, u_width * gl_FragCoord.z, 0);
    uvec4 texcoord = temp;
	if(f_axis == 0)	{

		texcoord.x = u_width - temp.z;
		texcoord.z = temp.x;
		texcoord.y = temp.y;

	} else if (f_axis == 1)	{

		texcoord.z = temp.y;
		texcoord.y = u_width - temp.z;
		texcoord.x = temp.x;

	}

    //out_Color = vec4(dominantAxis, 0.f);
    out_Color = vec4(texcoord) / 1000.f;
}
