#version 440 core

#include "common/extensions.glsl"
#include "common/materials.glsl"
#include "common/textures.glsl"
#include "common/instances.glsl"

layout(location = 0) out vec4 out_pos;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_color;
layout(location = 3) out float out_depth;

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

    vec3 normal;
    vec3 diffuse_color;
    vec3 specular_color;
    vec3 emissive_color;
    vec3 ambient_color;
    float glossiness;

    if (materials[materialID].hasDiffuseTex != 0) {
        diffuse_color = texture(uDiffuseTex, uv).rgb;
    } else {
        diffuse_color = materials[materialID].diffuseColor;
    }

    if (materials[materialID].hasSpecularTex != 0) {
        specular_color = texture(uSpecularTex, uv).rgb;
    } else {
        specular_color = materials[materialID].specularColor;
    }

    if (materials[materialID].hasEmissiveTex != 0) {
        emissive_color = texture(uEmissiveTex, uv).rgb;
    } else {
        emissive_color = materials[materialID].emissiveColor;
    }

    if (materials[materialID].hasAmbientTex != 0) {
        ambient_color = texture(uAmbientTex, uv).rgb;
    } else {
        ambient_color = materials[materialID].ambientColor;
    }

    if (materials[materialID].hasGlossyTex != 0) {
        glossiness = texture(uGlossyTex, uv).r;
    } else {
        glossiness = materials[materialID].glossiness;
    }


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

    const vec3 light_dir = vec3(0.0, 1.0, 0.0);

    const float n_dot_l = max(dot(light_dir, normal), 0.0);
    float spec = 0;
    if (n_dot_l > 0.0) {
        vec3 halfvec = normalize(light_dir + inData.viewdir);
        spec = pow(max(dot(normal, halfvec), 0.0), glossiness);
    }


    out_pos    = vec4(inData.wpos, 1);
    out_normal = normal;
    out_color  = vec3(diffuse_color * n_dot_l + specular_color * spec + 0.15 * ambient_color);
    out_depth  = gl_FragCoord.z;
}

