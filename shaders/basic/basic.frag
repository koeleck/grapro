#version 440 core

#extension GL_ARB_bindless_texture : require

#include "common/materials.glsl"
#include "common/textures.glsl"

layout(location = 0) out vec4 out_Color;

in vec3 vs_viewdir;

#ifdef HAS_NORMALS
in vec3 vs_normal;
#endif
#ifdef HAS_TEXCOORDS
in vec2 vs_uv;
#endif
#ifdef HAS_TANGENTS
in vec3 vs_tangent;
in vec3 vs_bitangent;
#endif

uniform uint uMaterialID;


void main()
{
    vec3 normal;
    vec3 diffuse_color;
    vec3 specular_color;
    vec3 emissive_color;
    vec3 ambient_color;
    float glossiness;

#ifdef HAS_TEXCOORDS
    if (materials[uMaterialID].hasAlphaTex != 0) {
        if (0.5 > texture(uAlphaTex, vs_uv).r) {
            discard;
        }
    }

    if (materials[uMaterialID].hasDiffuseTex != 0) {
        diffuse_color = texture(uDiffuseTex, vs_uv).rgb;
    } else {
        diffuse_color = materials[uMaterialID].diffuseColor;
    }

    if (materials[uMaterialID].hasSpecularTex != 0) {
        specular_color = texture(uSpecularTex, vs_uv).rgb;
    } else {
        specular_color = materials[uMaterialID].specularColor;
    }

    if (materials[uMaterialID].hasEmissiveTex != 0) {
        emissive_color = texture(uEmissiveTex, vs_uv).rgb;
    } else {
        emissive_color = materials[uMaterialID].emissiveColor;
    }

    if (materials[uMaterialID].hasAmbientTex != 0) {
        ambient_color = texture(uAmbientTex, vs_uv).rgb;
    } else {
        ambient_color = materials[uMaterialID].ambientColor;
    }

    if (materials[uMaterialID].hasGlossyTex != 0) {
        glossiness = texture(uGlossyTex, vs_uv).r;
    } else {
        glossiness = materials[uMaterialID].glossiness;
    }


#ifdef HAS_TANGENTS
    if (materials[uMaterialID].hasNormalTex != 0) {
        mat3 localToWorld;
        localToWorld[0] = vs_tangent;
        localToWorld[1] = vs_bitangent;
        localToWorld[2] = vs_normal;
        vec3 texNormal = texture(uNormalTex, vs_uv).rgb;
        texNormal.xy = texNormal.xy * 2.0 - 1.0;
        texNormal = normalize(texNormal);
        normal =  localToWorld * texNormal;
    } else {
        normal = vs_normal;
    }
#else
    normal = vs_normal;
#endif // NORMALS

#else
    normal = vs_normal;
    diffuse_color = materials[uMaterialID].diffuseColor;
    specular_color = materials[uMaterialID].specularColor;
    ambient_color = materials[uMaterialID].ambientColor;
    emissive_color = materials[uMaterialID].emissiveColor;
    glossiness = materials[uMaterialID].glossiness;
#endif

    vec3 light_dir = vec3(0.0, 1.0, 0.0);

    float n_dot_l = max(dot(light_dir, normal), 0.0);
    float spec = 0;
    if (n_dot_l > 0.0) {
        vec3 halfvec = normalize(light_dir + vs_viewdir);
        spec = pow(max(dot(normal, halfvec), 0.0), glossiness);
    }

    out_Color = vec4(diffuse_color * n_dot_l + specular_color * spec + 0.15 * ambient_color, 1.0);
}
