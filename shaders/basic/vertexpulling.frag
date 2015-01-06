#version 440 core

#include "common/extensions.glsl"
#include "common/materials.glsl"
#include "common/textures.glsl"
#include "common/instances.glsl"

layout(location = 0) out vec4 out_Color;

in vec3 vs_viewdir;
in vec3 vs_normal;
in vec2 vs_uv;
in vec3 vs_tangent;
in vec3 vs_bitangent;
flat in uint materialID;

in vec3 vs_color;

void main()
{
    if (materials[materialID].hasAlphaTex != 0) {
        if (0.5 > texture(uAlphaTex, vs_uv).r) {
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
        diffuse_color = texture(uDiffuseTex, vs_uv).rgb;
    } else {
        diffuse_color = materials[materialID].diffuseColor;
    }

    if (materials[materialID].hasSpecularTex != 0) {
        specular_color = texture(uSpecularTex, vs_uv).rgb;
    } else {
        specular_color = materials[materialID].specularColor;
    }

    if (materials[materialID].hasEmissiveTex != 0) {
        emissive_color = texture(uEmissiveTex, vs_uv).rgb;
    } else {
        emissive_color = materials[materialID].emissiveColor;
    }

    if (materials[materialID].hasAmbientTex != 0) {
        ambient_color = texture(uAmbientTex, vs_uv).rgb;
    } else {
        ambient_color = materials[materialID].ambientColor;
    }

    if (materials[materialID].hasGlossyTex != 0) {
        glossiness = texture(uGlossyTex, vs_uv).r;
    } else {
        glossiness = materials[materialID].glossiness;
    }


    if (materials[materialID].hasNormalTex != 0) {
        vec3 texNormal = texture(uNormalTex, vs_uv).rgb;
        texNormal.xy = texNormal.xy * 2.0 - 1.0;
        texNormal = normalize(texNormal);

        //mat3 localToWorld = mat3(vs_tangent, vs_bitangent, vs_normal);
        //normal =  localToWorld * texNormal;
        mat3 localToWorld_T = mat3(
                vs_tangent.x, vs_bitangent.x, vs_normal.x,
                vs_tangent.y, vs_bitangent.y, vs_normal.y,
                vs_tangent.z, vs_bitangent.z, vs_normal.z);
        normal = texNormal * localToWorld_T;

    } else {
        normal = vs_normal;
    }

    const vec3 light_dir = vec3(0.0, 1.0, 0.0);

    const float n_dot_l = max(dot(light_dir, normal), 0.0);
    float spec = 0;
    if (n_dot_l > 0.0) {
        vec3 halfvec = normalize(light_dir + vs_viewdir);
        spec = pow(max(dot(normal, halfvec), 0.0), glossiness);
    }

    out_Color = vec4(diffuse_color * n_dot_l + specular_color * spec + 0.15 * ambient_color, 1.0);
}

