#version 440 core

#include "common/materials.glsl"
#include "common/bindings.glsl"
#include "common/textures.glsl"
#include "common/compression.glsl"
#include "voxel.glsl"

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

vec3 getNormal()
{
    vec3 normal;
    if (materials[inData.materialID].hasNormalTex != 0) {
        vec3 texNormal = texture(uNormalTex, inData.uv).rgb;
        texNormal.xy = texNormal.xy * 2.0 - 1.0;
        texNormal = normalize(texNormal);

        const mat3 localToWorld_T = mat3(
                inData.tangent.x, inData.bitangent.x, inData.normal.x,
                inData.tangent.y, inData.bitangent.y, inData.normal.y,
                inData.tangent.z, inData.bitangent.z, inData.normal.z);
        normal = texNormal * localToWorld_T;
    } else {
        normal = inData.normal;
    }
    return normal;
}

vec3 getColor()
{
    vec3 color;
    if (materials[inData.materialID].hasDiffuseTex != 0) {
        color = texture(uDiffuseTex, inData.uv).rgb;
    } else {
        color = materials[inData.materialID].diffuseColor;
    }
    return color;
}

void main()
{

    if (any(lessThan(inData.position.xy, inData.AABB.xy)) ||
        any(greaterThan(inData.position.xy, inData.AABB.zw)))
    {
        return;
    }
    const uint materialID = inData.materialID;
    const vec2 uv = inData.uv;
    if (materials[materialID].hasAlphaTex != 0) {
        if (0.5 > texture(uAlphaTex, uv).r) {
            return;
        }
    }

    // material properties
    vec3 diffuse_color;
    if (materials[materialID].hasDiffuseTex != 0) {
        diffuse_color = texture(uDiffuseTex, uv).rgb;
    } else {
        diffuse_color = materials[materialID].diffuseColor;
    }
    vec3 normal;
    if (materials[materialID].hasNormalTex != 0) {
        vec3 texNormal = texture(uNormalTex, uv).rgb;
        texNormal.xy = texNormal.xy * 2.0 - 1.0;
        texNormal = normalize(texNormal);

        mat3 localToWorld_T = mat3(
                inData.tangent.x, inData.bitangent.x, inData.normal.x,
                inData.tangent.y, inData.bitangent.y, inData.normal.y,
                inData.tangent.z, inData.bitangent.z, inData.normal.z);
        normal = texNormal * localToWorld_T;
    } else {
        normal = inData.normal;
    }
    vec3 emissive_color;
    if (materials[materialID].hasEmissiveTex != 0) {
        emissive_color = texture(uEmissiveTex, uv).rgb;
    } else {
        emissive_color = materials[materialID].emissiveColor;
    }

    // save fragment
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
    vec3 unorm_pos = fma(pos, vec3(0.5), vec3(0.5));
    vec3 unorm_normal = fma(normal, vec3(0.5), vec3(0.5));
    uvec3 texcoord = uvec3(unorm_pos * float(uNumVoxels));

    texcoord = clamp(texcoord, uvec3(0), uvec3(uNumVoxels - 1));

    const uint idx = atomicCounterIncrement(uVoxelFragCount);

    voxelFragment[idx].position_xyz = packUInt3x10(texcoord);
    voxelFragment[idx].diff_rgb_normal_x = packUnorm4x8(vec4(diffuse_color, unorm_normal.x));
    voxelFragment[idx].normal_yz_emissive_rg = packUnorm4x8(vec4(unorm_normal.yz, emissive_color.rb));
    voxelFragment[idx].emissive_b = packUnorm4x8(vec4(emissive_color.b, vec3(0.0)));

}
