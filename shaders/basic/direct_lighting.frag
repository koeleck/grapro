#version 440 core

#include "common/gbuffer.glsl"
#include "common/compression.glsl"
#include "common/textures.glsl"
#include "common/lights.glsl"

layout(location = 0) out vec4 outFragColor;

in vec2 vsTexCoord;
uniform uint u_shadowsEnabled;

void readIn(out vec3 diffuse, out vec3 normal, out float spec,
        out float gloss, out vec3 emissive)
{
    ivec2 crd = ivec2(gl_FragCoord.xy);
    bool isBlack = ((crd.x & 1) == (crd.y & 1));

    // read in data
    vec4 col = texelFetch(uDiffuseNormalTex, crd, 0);

    normal = decompressNormal(col.zw);

    /* edge directed */
    vec2 a0 = texelFetchOffset(uDiffuseNormalTex, crd, 0, ivec2( 1,  0)).rg;
    vec2 a1 = texelFetchOffset(uDiffuseNormalTex, crd, 0, ivec2(-1,  0)).rg;
    vec2 a2 = texelFetchOffset(uDiffuseNormalTex, crd, 0, ivec2( 0,  1)).rg;
    vec2 a3 = texelFetchOffset(uDiffuseNormalTex, crd, 0, ivec2( 0, -1)).rg;
    float chroma = edge_filter(col.rg, a0, a1, a2, a3);

    col.b = chroma;
    col.rgb = (isBlack) ? col.rgb : col.rbg;
    diffuse = YCoCg2RGB(col.rgb);

    col = texelFetch(uSpecularGlossyEmissiveTex, crd, 0);

    spec = col.r;
    gloss = col.g;

    /* edge directed */
    a0 = texelFetchOffset(uSpecularGlossyEmissiveTex, crd, 0, ivec2( 1,  0)).ba;
    a1 = texelFetchOffset(uSpecularGlossyEmissiveTex, crd, 0, ivec2(-1,  0)).ba;
    a2 = texelFetchOffset(uSpecularGlossyEmissiveTex, crd, 0, ivec2( 0,  1)).ba;
    a3 = texelFetchOffset(uSpecularGlossyEmissiveTex, crd, 0, ivec2( 0, -1)).ba;
    chroma = edge_filter(col.ba, a0, a1, a2, a3);

    col.r = chroma;
    col.rgb = (isBlack) ? col.bar : col.bra;
    emissive = YCoCg2RGB(col.rgb);
}

void main()
{
    vec3 diffuse;
    vec3 emissive;
    vec3 normal;
    float specular;
    float glossy;

    readIn(diffuse, normal, specular, glossy, emissive);

    vec4 wpos = resconstructWorldPos(vsTexCoord);
    vec3 viewdir = normalize(cam.Position.xyz - wpos.xyz);

    vec3 result = vec3(0.0);
    for (int i = 0; i < numLights; ++i) {
        float attenuation;
        vec3 light_dir;
        const int type_texid = lights[i].type_texid;
        const bool isShadowcasting = (type_texid & LIGHT_IS_SHADOWCASTING) != 0 && u_shadowsEnabled != 0;

        // TODO max dist
        if ((type_texid & LIGHT_TYPE_DIRECTIONAL) != 0) {
            light_dir = -lights[i].direction;
            attenuation = 1.0;
            if (isShadowcasting) {
                int layer = (type_texid & LIGHT_TEXID_BITS);
                vec4 tmp = lights[i].ProjViewMatrix * wpos;
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
            const vec3 diff = wpos.xyz - lights[i].position;
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

                vec4 tmp = lights[i].ProjViewMatrix * wpos;
                tmp.xyz = (tmp.xyz / tmp.w) * 0.5 + 0.5;

                vec4 tmp2 = lights[i].ProjViewMatrix * (wpos + vec4(normalOffset, 1.0));
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
            const vec3 diff = wpos.xyz - lights[i].position;
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
            vec3 halfvec = normalize(light_dir + viewdir);
            spec = pow(max(dot(normal, halfvec), 0.0), glossy);
        }

        result += attenuation * lights[i].intensity * (diffuse * n_dot_l + vec3(specular) * spec);
    }
    result += emissive;

    outFragColor = vec4(result, 1.0);
}
