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
//uniform sampler2D uTexHandle;

void main()
{
    vec3 normal;
    vec3 diffuse_color;
    vec3 specular_color;
    vec3 emissive_color;
    vec3 ambient_color;
    float glossiness;

#ifdef HAS_TEXCOORDS
    uvec2 tex = materials[uMaterialID].alphaTex;
    if (!all(equal(tex, uvec2(0)))) {
        if (0.5 > texture(sampler2D(tex), vs_uv).r) {
            discard;
        }
    }

    tex = materials[uMaterialID].diffuseTex;
    if (!all(equal(tex, uvec2(0)))) {
        diffuse_color = texture(sampler2D(tex), vs_uv).rgb;
    } else {
        diffuse_color = materials[uMaterialID].diffuseColor;
    }

    tex = materials[uMaterialID].specularTex;
    if (!all(equal(tex, uvec2(0)))) {
        specular_color = texture(sampler2D(tex), vs_uv).rgb;
    } else {
        specular_color = materials[uMaterialID].specularColor;
    }

    tex = materials[uMaterialID].emissiveTex;
    if (!all(equal(tex, uvec2(0)))) {
        emissive_color = texture(sampler2D(tex), vs_uv).rgb;
    } else {
        emissive_color = materials[uMaterialID].emissiveColor;
    }

    tex = materials[uMaterialID].ambientTex;
    if (!all(equal(tex, uvec2(0)))) {
        ambient_color = texture(sampler2D(tex), vs_uv).rgb;
    } else {
        ambient_color = materials[uMaterialID].ambientColor;
    }

    tex = materials[uMaterialID].glossyTex;
    if (!all(equal(tex, uvec2(0)))) {
        glossiness = texture(sampler2D(tex), vs_uv).r;
    } else {
        glossiness = materials[uMaterialID].glossiness;
    }


#ifdef HAS_TANGENTS
    tex = materials[uMaterialID].normalTex;
    if (!all(equal(tex, uvec2(0)))) {
        mat3 localToWorld;
        localToWorld[0] = vs_tangent;
        localToWorld[1] = vs_bitangent;
        localToWorld[2] = vs_normal;
        vec3 texNormal = texture(sampler2D(tex), vs_uv).rgb;
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
