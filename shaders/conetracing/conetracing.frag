#version 440 core

#include "common/gbuffer.glsl"
#include "common/compression.glsl"
#include "common/textures.glsl"
#include "common/lights.glsl"
#include "tree/voxel.glsl"

/******************************************************************************/

layout(location = 0) out vec4 outFragColor;
layout(location = 1) uniform uint u_voxelDim;
layout(location = 2) uniform vec3 u_bboxMin;
layout(location = 3) uniform vec3 u_bboxMax;
layout(location = 4) uniform uint u_screenwidth;
layout(location = 5) uniform uint u_screenheight;
layout(location = 6) uniform uint u_treeLevels;
layout(location = 7) uniform uint u_coneGridSize;
layout(location = 8) uniform uint u_numSteps;

in vec2 vsTexCoord;

/******************************************************************************/

const float voxelSize = (u_bboxMax.x - u_bboxMin.x) / float(u_voxelDim);

/******************************************************************************/

#define M_PI 3.1415926535897932384626433832795

struct Cone
{
    vec3 dir;
    float angle; // in degrees
};

struct ONB
{
    vec3 N, S, T;
};

/******************************************************************************/

float degreesToRadians(in float deg)
{
    return deg * M_PI / 180.0;
}

/******************************************************************************/

float radiansToDegrees(in float rad)
{
    return rad * 180.0 / M_PI;
}

/******************************************************************************/

float coneRadiusAtDistance(in Cone cone, in float distance)
{
    const float angle  = cone.angle * 0.5;
    return distance * tan(degreesToRadians(angle));
}

/******************************************************************************/

vec3 uniformHemisphereSampling(in float ux, in float uy)
{
    clamp(ux, 0.0, 1.0);
    clamp(uy, 0.0, 1.0);
    const float r = sqrt(1.0 - ux * ux);
    const float phi = 2.0 * M_PI * uy;
    return vec3(cos(phi) * r, sin(phi) * r, ux);
}

/******************************************************************************/

ONB toONB(vec3 normal)
{
    ONB onb;
    onb.N = normal;
    if(abs(normal.x) > abs(normal.z)) {
        onb.S.x = -normal.y;
        onb.S.y = normal.x;
        onb.S.z = 0.0;
    } else {
        onb.S.x = 0.0;
        onb.S.y = -normal.z;
        onb.S.z = normal.y;
    }

    onb.S = normalize(onb.S);
    onb.T = cross(onb.S, onb.N);
    return onb;
}

/******************************************************************************/

vec3 toWorld(ONB onb, vec3 v)
{
    return onb.S * v.x + onb.T * v.y + onb.N * v.z;
}

/******************************************************************************/

vec4 getColor(uint maxLevel, vec3 wpos)
{
    vec3 vpos = float(u_voxelDim) * (wpos - u_bboxMin) / (u_bboxMax - u_bboxMin);
    uvec3 pos = min(uvec3(vpos), uvec3(u_voxelDim - 1));

    uint voxel_dim = u_voxelDim;;
    uvec3 umin = uvec3(0u);

    uint currentNode = 0;

    // iterate through all tree levels
    for (uint i = 0u; i <= maxLevel; ++i) {
        uint nodeptr = octree[currentNode].id;
        if ((nodeptr & 0x80000000u) == 0)
            break;

        voxel_dim /= 2u;

        // find subnode
        uvec3 subnode = clamp((pos - umin) / voxel_dim, uvec3(0u), uvec3(1u));

        // get child pointer by ignoring the node's ptr MSB
        uint childidx = uint(nodeptr & 0x7FFFFFFFu);
        // determine index of subnode
        childidx += subnode.z * 4u + subnode.y * 2u + subnode.x;

        umin += voxel_dim * subnode;

        currentNode = childidx;
    }
    if ((octree[currentNode].id & 0x80000000u) == 0)
        return vec4(0.0);

    //vec3 coord = getBrickTexCoord(currentNode, vpos - floor(vpos));
    //return texture(uOctree3DTex, coord);
    return imageLoad(octreeBrickTex, getBrickCoord(currentNode));
}

/******************************************************************************/

vec3 traceCone(in vec3 origin, in vec3 direction, in float angle, in uint steps)
{
    const float tan_a = tan(angle / 2.0);

    float stepSize = (u_bboxMax.x - u_bboxMin.x) / float(steps);

    vec3 result = vec3(0.0);
    float alpha = 1.0;
    float dist = stepSize;
    for (uint i = 0; i < steps; ++i) {
        const vec3 pos = origin + dist * direction;
        const float diameter = 2.0 * (tan_a * dist);

        // calculate mipmap level
        //const uint level = clamp(uint(log2(diameter / float(voxelSize))) - 1, 0, u_treeLevels - 1);
        //const vec4 color = getColor(level, pos);

        // quadrilinear
        float level = clamp(log2(diameter / voxelSize), 0.0, float(u_treeLevels - 1));
        vec4 color0 = getColor(uint(floor(level)), pos);
        vec4 color1 = getColor(uint(floor(level + 1.1)), pos);
        float fac = level - floor(level);
        vec4 color = fac * color0 + (1.0 - fac) * color1;


        result += alpha * color.a * color.rgb;

        if(color.a >= 0.99) {
            break;
        }

        alpha *= (1.0 - color.a);
        dist += stepSize;
    }

    return result;
}

/******************************************************************************/


vec3 calculateDiffuseColor(const vec3 normal, const vec3 pos)
{
    vec3 totalColor = vec3(0.0);
    const float step = (1.0 / float(u_coneGridSize));

    ONB onb = toONB(normal);
    const float angle = degreesToRadians(179.0 / float(u_coneGridSize));

    for (uint y = 0; y < u_coneGridSize; ++y) {
        const float uy = (0.5 + float(y)) * step;
        for (uint x = 0; x < u_coneGridSize; ++x) {
            const float ux = (0.5 + float(x)) * step;

            // create the cone
            vec3 v = uniformHemisphereSampling(ux, uy); //  do random here if you want
            vec3 dir = normalize(toWorld(onb, v));

            float d = abs(dot(normal, dir));

            totalColor += d * traceCone(pos, dir, angle, u_numSteps);
        }

    }
    return 0.8 * totalColor / float(u_coneGridSize * u_coneGridSize);
}

/******************************************************************************/

void readIn(out vec3 diffuse, out vec3 normal, out float spec,
        out float gloss, out vec3 emissive)
{
    ivec2 crd = ivec2(gl_FragCoord.xy);

    if (texelFetch(uDepthTex, crd, 0).r == 1.0)
        discard;

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

/******************************************************************************/

void main()
{
    vec3 diffuse;
    vec3 emissive;
    vec3 normal;
    float specular;
    float glossy;

    readIn(diffuse, normal, specular, glossy, emissive);
    vec4 wpos = resconstructWorldPos(vsTexCoord);

    // specular
    vec3 incident = normalize(wpos.xyz - cam.Position.xyz);
    vec3 refl = reflect(incident, normal);


    float angle = degreesToRadians(10.0);
    vec3 spec = specular * traceCone(wpos.xyz, refl, angle, u_numSteps);
    outFragColor = vec4(spec, 1.0);

    //outFragColor = vec4(calculateDiffuseColor(normal, wpos.xyz), 1.0);
    //outFragColor = vec4(spec + 2.5 * calculateDiffuseColorAO(normal, wpos.xyz), 1.0);
}

/******************************************************************************/
