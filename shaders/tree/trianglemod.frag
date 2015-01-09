#version 440 core

layout(location = 0) out vec4 out_Color;

flat in vec3 dominantAxis;
flat in int f_axis;
flat in vec4 f_AABB;

in vec3 f_pos;

//atomic counter
layout(binding = 0, offset = 0) uniform atomic_uint u_voxelFragCount;

uniform int u_width;
uniform int u_height;

layout(binding = 0, rgb10_a2) uniform restrict image2D imgVoxelPos;
layout(binding = 1, rgba8) uniform restrict image2D imgVoxelKd;
layout(binding = 2, rgba16f) uniform restrict image2D imgVoxelNormal;

void main()
{
    if(f_pos.x < f_AABB.x || f_pos.y < f_AABB.y || f_pos.x > f_AABB.z || f_pos.y > f_AABB.w) {
		discard;
    }

    uvec4 temp = uvec4(gl_FragCoord.xy, u_width * gl_FragCoord.z, 0);
    uvec4 texcoord = temp; // default: f_axis == 2
	if(f_axis == 0)	{

		texcoord.x = u_width - temp.z;
		texcoord.z = temp.x;
		texcoord.y = temp.y;

	} else if (f_axis == 1)	{

		texcoord.z = temp.y;
		texcoord.y = u_width - temp.z;
		texcoord.x = temp.x;

	}

	uint idx = atomicCounterIncrement(u_voxelFragCount);

    //out_Color = vec4(dominantAxis, 0.f);
    out_Color = vec4(idx) / 10000.f;
}
