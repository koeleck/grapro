#version 440 core

#include "common/materials.glsl"
#include "common/bindings.glsl"
#include "common/textures.glsl"
#include "common/voxel.glsl"

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
layout(binding = 0) uniform atomic_uint uVoxelFragCount;

uniform int uNumVoxels;

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

    normalize(m_normal);

}

void setColor() {

    if (materials[inData.materialID].hasDiffuseTex != 0) {
        m_diffuse_color = texture(uDiffuseTex, inData.uv).rgb;
    } else {
        m_diffuse_color = materials[inData.materialID].diffuseColor;
    }

    if (materials[inData.materialID].hasEmissiveTex != 0) {
        m_emissive_color = texture(uEmissiveTex, inData.uv).rgb;
    } else {
        m_emissive_color = materials[inData.materialID].emissiveColor;
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
    uvec3 texcoord = uvec3((pos * 0.5 + 0.5) * float(uNumVoxels));

    texcoord = clamp(texcoord, uvec3(0), uvec3(uNumVoxels - 1));

    const uint idx = atomicCounterIncrement(uVoxelFragCount);

    voxel[idx].position = convertPosition(texcoord.xyz);
    voxel[idx].color = convertColor(m_diffuse_color);
    voxel[idx].normal = packUnorm4x8(vec4(m_normal, 0));
    voxel[idx].emissive = convertColor(m_emissive_color);

}
