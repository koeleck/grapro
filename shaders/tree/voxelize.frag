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
layout(binding = 0) uniform atomic_uint uVoxelFragCount;

// voxel buffer
struct voxelStruct {
    uvec4 position;
    vec4 color;
    vec4 normal;
};

layout (std430, binding = VOXEL_BINDING) buffer voxelBlock {
    voxelStruct voxel[];
};

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

    if(inData.position.x < inData.AABB.x  || inData.position.y < inData.AABB.y ||
        inData.position.x > inData.AABB.z || inData.position.y > inData.AABB.w)
    {
        discard;
    }

    /*
    const uvec4 temp = uvec4(gl_FragCoord.xy,
            float(uNumVoxels) * gl_FragCoord.z, 0);
    uvec4 texcoord = temp; // default: inData.axis == 2
    if(inData.axis == 0) {
        texcoord.x = uNumVoxels - temp.z;
        texcoord.z = temp.x;
    } else if (inData.axis == 1) {
        texcoord.z = temp.y;
        texcoord.y = uNumVoxels - temp.z;
    }
    */
    uvec3 texcoord = uvec3((inData.position * 0.5 + 0.5) * float(uNumVoxels));
    if (inData.axis == 0) {
        uint tmp = texcoord.x;
        texcoord.x = uNumVoxels - texcoord.z;
        texcoord.z = tmp;
    } else if (inData.axis == 1) {
        uint tmp = texcoord.y;
        texcoord.y = uNumVoxels - texcoord.z;
        texcoord.z = tmp;
    }

    setNormal();
    setColor();

    const uint idx = atomicCounterIncrement(uVoxelFragCount);

    voxel[idx].position = uvec4(texcoord.xyz, 0);
    voxel[idx].color = vec4(m_diffuse_color, 0.f);
    voxel[idx].normal = vec4(m_normal, 0.f);

    out_Color = vec4(1.f);

}
