#version 440 core

#include "common/materials.glsl"
#include "common/bindings.glsl"
#include "common/textures.glsl"

layout(location = 0) out vec4 out_Color;

flat in vec3 dominantAxis;
flat in int f_axis;
flat in vec4 f_AABB;

flat in uint materialID;

in vec3 f_pos;
in vec3 vs_normal;
in vec2 vs_uv;
in vec3 vs_tangent;
in vec3 vs_bitangent;

// atomic counter
layout(binding = 0, offset = 0) uniform atomic_uint u_voxelFragCount;

// voxel buffer
struct voxelStruct {
	uvec4 position;
	vec4 color;
	vec4 normal;
};

layout (std430, binding = VOXEL_BINDING) buffer voxelBlock {
	voxelStruct voxel[];
};

uniform int u_width;
uniform int u_height;
uniform sampler2D u_colorTex;

vec3 m_normal;
vec3 m_diffuse_color;
vec3 m_specular_color;
vec3 m_emissive_color;
vec3 m_ambient_color;

void setNormal() {

	if (materials[materialID].hasNormalTex != 0) {
        vec3 texNormal = texture(uNormalTex, vs_uv).rgb;
        texNormal.xy = texNormal.xy * 2.0 - 1.0;
        texNormal = normalize(texNormal);

        const mat3 localToWorld_T = mat3(
                vs_tangent.x, vs_bitangent.x, vs_normal.x,
                vs_tangent.y, vs_bitangent.y, vs_normal.y,
                vs_tangent.z, vs_bitangent.z, vs_normal.z);
        m_normal = texNormal * localToWorld_T;
    } else {
        m_normal = vs_normal;
    }

}

void setColor() {

	if (materials[materialID].hasDiffuseTex != 0) {
        m_diffuse_color = texture(uDiffuseTex, vs_uv).rgb;
    } else {
        m_diffuse_color = materials[materialID].diffuseColor;
    }

}

void main()
{

    if(f_pos.x < f_AABB.x || f_pos.y < f_AABB.y || f_pos.x > f_AABB.z || f_pos.y > f_AABB.w) {
		discard;
    }

    const uvec4 temp = uvec4(gl_FragCoord.xy, u_width * gl_FragCoord.z, 0);
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

	setNormal();
	setColor();

	const uint idx = atomicCounterIncrement(u_voxelFragCount);

	voxel[idx].position = uvec4(texcoord.xyz, 0);
	voxel[idx].color = vec4(m_diffuse_color, 0.f);
	voxel[idx].normal = vec4(m_normal, 0.f);

    out_Color = vec4(1.f);

}
