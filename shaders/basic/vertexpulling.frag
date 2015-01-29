#version 440 core

#include "common/extensions.glsl"
#include "common/materials.glsl"
#include "common/textures.glsl"
#include "common/instances.glsl"
#include "common/compression.glsl"

layout(location = 0) out vec4 out_DiffuseNormal;
layout(location = 1) out vec4 out_SpecGlossEmissive;

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
        diffuse_color = pow(texture(uDiffuseTex, uv).rgb, vec3(2.2));
    } else {
        diffuse_color = materials[materialID].diffuseColor;
    }

    if (materials[materialID].hasSpecularTex != 0) {
        specular_color = texture(uSpecularTex, uv).rgb;
    } else {
        specular_color = materials[materialID].specularColor;
    }

    if (materials[materialID].hasEmissiveTex != 0) {
        emissive_color = pow(texture(uEmissiveTex, uv).rgb, vec3(2.2));
    } else {
        emissive_color = materials[materialID].emissiveColor;
    }

    if (materials[materialID].hasAmbientTex != 0) {
        ambient_color = pow(texture(uAmbientTex, uv).rgb, vec3(2.2));
    } else {
        ambient_color = materials[materialID].ambientColor;
    }

    if (materials[materialID].hasGlossyTex != 0) {
        glossiness = 255.0 * texture(uGlossyTex, uv).r;
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

    const float AMBIENT_FAC = 0.02;
    vec3 diff_YCoCg = RGB2YCoCg(diffuse_color);
    vec3 em_YCoCg = RGB2YCoCg(AMBIENT_FAC * ambient_color + emissive_color);
    float spec = RGB2YCoCg(specular_color).x;

    ivec2 crd = ivec2(gl_FragCoord.xy);
    bool isBlack = ((crd.x & 1) == (crd.y & 1));

    out_DiffuseNormal.xy = (isBlack) ? diff_YCoCg.rg : diff_YCoCg.rb;
    out_DiffuseNormal.zw = compressNormal(normal);
    out_SpecGlossEmissive.x = spec;
    out_SpecGlossEmissive.y = glossiness;
    out_SpecGlossEmissive.zw = (isBlack) ? em_YCoCg.rg : em_YCoCg.rb;

}

