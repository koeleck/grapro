#version 440 core

#include "common/gbuffer.glsl"
#include "common/compression.glsl"
#include "common/textures.glsl"
#include "common/lights.glsl"

layout(location = 0) out vec4 outFragColor;

in vec2 vsTexCoord;

layout(location = 0) uniform bool u_shadowsEnabled;

float get2DShadow(in const mat4 ProjViewMatrix_T,
        in const float fovFactor,
        in const vec3 lightDir,
        in const vec3 toLight,
        in const vec3 diff,
        in const vec3 normal,
        in const vec4 wpos,
        in const int layer)
{
    // normal offset
    float shadowMapTexelSize = 2.0 / textureSize(uShadowMapTex, 0).x;

    // scale normal offset by shadow depth
    shadowMapTexelSize *= abs(dot(diff, lightDir)) * fovFactor;

    float cosLightAngle = abs(dot(toLight, normal));

    float normalOffsetScale = clamp(1.0 - cosLightAngle, 0.0, 1.0);
    const float shadowNormalOffset = 50.0; // TODO
    normalOffsetScale *= shadowNormalOffset * shadowMapTexelSize;
    vec4 shadowOffset = vec4(normal * normalOffsetScale, 0.0);

    // only uv offset
    vec4 lP = wpos * ProjViewMatrix_T;
    vec4 lUVOffsetP = (wpos + shadowOffset) * ProjViewMatrix_T;
    lP.xy = lUVOffsetP.xy;

    // slope scale
    float sinLightAngle = sqrt(1.0 - cosLightAngle*cosLightAngle);
    float slope = sinLightAngle / max(cosLightAngle, 0.00001);
    const float slopeScaleBias = 0.09; // TODO
    float shadowBias = 0.09; // TODO
    shadowBias += slope * slopeScaleBias;
    lP.z -= shadowBias;

    lP.xyz = (lP.xyz / lP.w) * 0.5 + 0.5;

    vec4 texcoord = vec4(lP.xy, float(layer), lP.z);

    // PCF:
    return  (
                textureOffset(uShadowMapTex, texcoord, ivec2(-1, -1)) +
                textureOffset(uShadowMapTex, texcoord, ivec2( 0, -1)) +
                textureOffset(uShadowMapTex, texcoord, ivec2( 1, -1)) +
                textureOffset(uShadowMapTex, texcoord, ivec2(-1,  0)) +
                textureOffset(uShadowMapTex, texcoord, ivec2( 0,  0)) +
                textureOffset(uShadowMapTex, texcoord, ivec2( 1,  0)) +
                textureOffset(uShadowMapTex, texcoord, ivec2(-1,  1)) +
                textureOffset(uShadowMapTex, texcoord, ivec2( 0,  1)) +
                textureOffset(uShadowMapTex, texcoord, ivec2( 1,  1))
            ) / 9.0;
}

float get3DShadow(in const vec3 lightPos,
        in const float fovFactor,
        in const vec2 depthFactor,
        in const vec3 toLight,
        in const vec3 diff,
        in const vec3 normal,
        in vec3 wpos,
        in const int layer)
{
    // normal offset
    float shadowMapTexelSize = 2.0 / textureSize(uShadowCubeMapTex, 0).x;

    // scale normal offset by shadow depth
    shadowMapTexelSize *= length(diff) * fovFactor;

    float cosLightAngle = dot(toLight, normal);

    float normalOffsetScale = clamp(1.0 - cosLightAngle, 0.0, 1.0);
    const float shadowNormalOffset = 10.0; // TODO
    normalOffsetScale *= shadowNormalOffset * shadowMapTexelSize;
    vec3 shadowOffset = normal * normalOffsetScale;
    wpos += shadowOffset;

    // slope scale
    float sinLightAngle = sqrt(1.0 - cosLightAngle*cosLightAngle);
    float slope = sinLightAngle / max(cosLightAngle, 0.00001);
    const float cubeSlopeScaleBias = 0.5; // TODO
    float cubeShadowBias = 50.19; // TODO
    cubeShadowBias += slope * cubeSlopeScaleBias;

    wpos += toLight * cubeShadowBias;

    vec3 absDiff = abs(diff);
    const float abs_z = max(absDiff.x, max(absDiff.y, absDiff.z));
    // see lights.cpp for depthFactor
    const float depth = depthFactor.x + depthFactor.y / abs_z;


    vec3 dir = wpos - lightPos;

    // hack: find perpendicular vector
    vec3 offDir0 = 2.0 * shadowMapTexelSize * normalize(cross(dir, vec3(0.0, 0.0, 1.0)));
    vec3 offDir1 = 2.0 * shadowMapTexelSize * normalize(cross(dir, offDir0));

    //vec4 texcoord = vec4(dir, layer);
    //return texture(uShadowCubeMapTex, texcoord, depth);
    return
        (
            texture(uShadowCubeMapTex, vec4(dir - offDir0 - offDir1, layer), depth) +
            texture(uShadowCubeMapTex, vec4(dir           - offDir1, layer), depth) +
            texture(uShadowCubeMapTex, vec4(dir + offDir0 - offDir1, layer), depth) +
            texture(uShadowCubeMapTex, vec4(dir - offDir0          , layer), depth) +
            texture(uShadowCubeMapTex, vec4(dir                    , layer), depth) +
            texture(uShadowCubeMapTex, vec4(dir + offDir0          , layer), depth) +
            texture(uShadowCubeMapTex, vec4(dir - offDir0 - offDir1, layer), depth) +
            texture(uShadowCubeMapTex, vec4(dir           - offDir1, layer), depth) +
            texture(uShadowCubeMapTex, vec4(dir + offDir0 - offDir1, layer), depth) +
            texture(uShadowCubeMapTex, vec4(dir - offDir0 + offDir1, layer), depth) +
            texture(uShadowCubeMapTex, vec4(dir           + offDir1, layer), depth) +
            texture(uShadowCubeMapTex, vec4(dir + offDir0 + offDir1, layer), depth)
        ) / 9.0;
}

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
        const bool isShadowcasting = u_shadowsEnabled &&
            ((type_texid & LIGHT_IS_SHADOWCASTING) != 0);

        // TODO max dist
        if ((type_texid & LIGHT_TYPE_DIRECTIONAL) != 0) {
            light_dir = -lights[i].direction;
            attenuation = 1.0;
            if (isShadowcasting) {
                int layer = (type_texid & LIGHT_TEXID_BITS);
                attenuation *= get2DShadow(lights[i].ProjViewMatrix_T,
                        lights[i].fovFactor,
                        lights[i].direction,
                        light_dir,
                        light_dir,
                        normal,
                        wpos,
                        layer);
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
                int layer = (type_texid & LIGHT_TEXID_BITS);
                attenuation *= get2DShadow(lights[i].ProjViewMatrix_T,
                        lights[i].fovFactor,
                        lights[i].direction,
                        light_dir,
                        diff,
                        normal,
                        wpos,
                        layer);
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
                attenuation *= get3DShadow(lights[i].position,
                        lights[i].fovFactor,
                        lights[i].direction.xy, // see lights.cpp why
                        light_dir,
                        diff,
                        normal,
                        wpos.xyz,
                        layer);
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
