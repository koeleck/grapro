#version 440 core

#include "common/materials.glsl"
#include "common/bindings.glsl"
#include "common/textures.glsl"

layout(location = 0) out vec4 out_Color;

in VertexFragmentData
{
    flat vec4 AABB;
    vec3 position;
    flat float axis;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    flat uint materialID;
    vec2 uv;
} inData;

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

uniform int u_numVoxels;

vec3 m_normal;
vec3 m_diffuse_color;
vec3 m_specular_color;
vec3 m_emissive_color;
vec3 m_ambient_color;

void setNormal() {

	if (materials[inData.materialID].hasNormalTex != 0) {
        vec3 texNormal = texture(uNormalTex, inData.uv).rgb;
        texNormal.xy = texNormal.xy * 2.0 - 1.0;
        texNormal = normalize(texNormal);

        const mat3 localToWorld_T = mat3(
                inData.tangent.x, inData.bitangent.x, inData.normal.x,
                inData.tangent.y, inData.bitangent.y, inData.normal.y,
                inData.tangent.z, inData.bitangent.z, inData.normal.z);
        m_normal = texNormal * localToWorld_T;
    } else {
        m_normal = inData.normal;
    }

}

void setColor() {

	if (materials[inData.materialID].hasDiffuseTex != 0) {
        m_diffuse_color = texture(uDiffuseTex, inData.uv).rgb;
    } else {
        m_diffuse_color = materials[inData.materialID].diffuseColor;
    }

}

void main()
{

    if (any(lessThan(inData.position.xy, inData.AABB.xy)) ||
        any(greaterThan(inData.position.xy, inData.AABB.zw)))
    {
        return;
    }

	setNormal();
	setColor();

    vec3 pos = inData.position.xyz;
    if (inData.axis == 0) {
        float tmp = pos.x;
        pos.x = pos.z;
        pos.z = tmp;
    } else if (inData.axis == 1) {
        float tmp = pos.y;
        pos.y = pos.z;
        pos.z = tmp;
    }
    uvec3 texcoord = uvec3((pos * 0.5 + 0.5) * float(u_numVoxels));

    texcoord = clamp(texcoord, uvec3(0), uvec3(u_numVoxels - 1));

	const uint idx = atomicCounterIncrement(u_voxelFragCount);

	voxel[idx].position = uvec4(texcoord.xyz, 0);
    voxel[idx].color = vec4(m_diffuse_color, 0.f);
    voxel[idx].normal = vec4(m_normal, 0.f);

}
