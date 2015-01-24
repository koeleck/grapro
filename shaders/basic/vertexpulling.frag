#version 440 core

#include "common/extensions.glsl"
#include "common/materials.glsl"
#include "common/textures.glsl"
#include "common/instances.glsl"
#include "common/lights.glsl"

layout(location = 0) out vec4 out_Color;

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

    //vec3 result = vec3(0.15 * ambient_color);
    vec3 result = vec3(0.0);
    for (int i = 0; i < numLights; ++i) {
        float attenuation;
        vec3 light_dir;
        const int type_texid = lights[i].type_texid;
        const bool isShadowcasting = (type_texid & LIGHT_IS_SHADOWCASTING) != 0;

        // TODO max dist
        if ((type_texid & LIGHT_TYPE_DIRECTIONAL) != 0) {
            light_dir = -lights[i].direction;
            attenuation = 1.0;
            if (isShadowcasting) {
                int layer = (type_texid & LIGHT_TEXID_BITS);
                vec4 tmp = lights[i].ProjViewMatrix * vec4(inData.wpos, 1.0);
                tmp.xyz = (tmp.xyz / tmp.w) * 0.5 + 0.5;
                vec4 texcoord = vec4(tmp.xy, float(layer), tmp.z);
                //attenuation *= texture(uShadowMapTex, texcoord);
                // PCF:
                attenuation *= (textureOffset(uShadowMapTex, texcoord, ivec2(-2, -2)) +
                                textureOffset(uShadowMapTex, texcoord, ivec2( 0, -2)) +
                                textureOffset(uShadowMapTex, texcoord, ivec2( 2, -2)) +
                                textureOffset(uShadowMapTex, texcoord, ivec2(-2,  0)) +
                                textureOffset(uShadowMapTex, texcoord, ivec2( 0,  0)) +
                                textureOffset(uShadowMapTex, texcoord, ivec2( 2,  0)) +
                                textureOffset(uShadowMapTex, texcoord, ivec2(-2,  2)) +
                                textureOffset(uShadowMapTex, texcoord, ivec2( 0,  2)) +
                                textureOffset(uShadowMapTex, texcoord, ivec2( 2,  2))) / 9.0;
            }
        } else if ((type_texid & LIGHT_TYPE_SPOT) != 0) {
            const vec3 diff = inData.wpos - lights[i].position;
            const float dist = length(diff);
            const vec3 dir = diff / dist;

            light_dir = -dir;
            attenuation = smoothstep(lights[i].angleOuterCone, lights[i].angleInnerCone,
                    dot(dir, lights[i].direction)) /
                    (lights[i].constantAttenuation + lights[i].linearAttenuation * dist +
                     lights[i].quadraticAttenuation * dist * dist);
            if (isShadowcasting) {
                // normal offset
                float normalOffsetScale = 1.0 - max(dot(light_dir, normal), 0.0);
                vec3 normalOffset = normal * 50.0 * normalOffsetScale;

                int layer = (type_texid & LIGHT_TEXID_BITS);

                vec4 tmp = lights[i].ProjViewMatrix * vec4(inData.wpos, 1.0);
                tmp.xyz = (tmp.xyz / tmp.w) * 0.5 + 0.5;

                vec4 tmp2 = lights[i].ProjViewMatrix * vec4(inData.wpos + normalOffset, 1.0);
                tmp.xy = (tmp2.xy / tmp2.w) * 0.5 + 0.5;

                vec4 texcoord = vec4(tmp.xy, float(layer), tmp.z);
                //attenuation *= texture(uShadowMapTex, texcoord);
                // PCF:
                attenuation *= (textureOffset(uShadowMapTex, texcoord, ivec2(-2, -2)) +
                                textureOffset(uShadowMapTex, texcoord, ivec2( 0, -2)) +
                                textureOffset(uShadowMapTex, texcoord, ivec2( 2, -2)) +
                                textureOffset(uShadowMapTex, texcoord, ivec2(-2,  0)) +
                                textureOffset(uShadowMapTex, texcoord, ivec2( 0,  0)) +
                                textureOffset(uShadowMapTex, texcoord, ivec2( 2,  0)) +
                                textureOffset(uShadowMapTex, texcoord, ivec2(-2,  2)) +
                                textureOffset(uShadowMapTex, texcoord, ivec2( 0,  2)) +
                                textureOffset(uShadowMapTex, texcoord, ivec2( 2,  2))) / 9.0;
            }
        } else { // POINT
            const vec3 diff = inData.wpos - lights[i].position;
            const float dist = length(diff);
            const vec3 dir = diff / dist;
            light_dir = -dir;
            attenuation = 1.0 /
                    (lights[i].constantAttenuation + lights[i].linearAttenuation * dist +
                     lights[i].quadraticAttenuation * dist * dist);
            if (isShadowcasting) {
                int layer = (type_texid & LIGHT_TEXID_BITS);
                vec3 absDiff = abs(diff);
                const float abs_z = max(absDiff.x, max(absDiff.y, absDiff.z));
                const float f = 2000.0; const float n = 1.0;
                // see src/core/light.cpp:
                const float depth = lights[i].direction.x + lights[i].direction.y / abs_z;

                vec4 texcoord = vec4(dir, layer);
                attenuation *= texture(uShadowCubeMapTex, texcoord, depth);
            }
        }

        const float n_dot_l = max(dot(light_dir, normal), 0.0);
        float spec = 0;
        if (n_dot_l > 0.0) {
            vec3 halfvec = normalize(light_dir + inData.viewdir);
            spec = pow(max(dot(normal, halfvec), 0.0), glossiness);
        }

        result += attenuation * lights[i].intensity * (diffuse_color * n_dot_l + specular_color * spec);
    }
    result += emissive_color;

    out_Color = vec4(result , 1.0);
}

