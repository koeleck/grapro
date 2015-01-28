#version 440 core

#include "common/extensions.glsl"
#include "common/materials.glsl"
#include "common/textures.glsl"
#include "common/instances.glsl"

layout(location = 0) out vec4 out_diffuse;

in VertexData
{
    vec3 wpos;
    vec3 viewdir;
    vec3 normal;
    vec2 uv;
    vec3 tangent;
    vec3 bitangent;
    flat uint materialID;
} inData;

void main()
{
    const uint materialID = inData.materialID;
    const vec2 uv = inData.uv;
    if (materials[materialID].hasAlphaTex != 0) {
        if (0.5 > texture(uAlphaTex, uv).r) {
            discard;
        }
    }

    if (materials[materialID].hasDiffuseTex != 0) {
        out_diffuse = vec4(texture(uDiffuseTex, uv).rgb, 0);
    } else {
        out_diffuse = vec4(materials[materialID].diffuseColor, 0);
    }
}

