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

float degreesToRadians(float deg)
{
    return deg * M_PI / 180.f;
}

/******************************************************************************/

float radiansToDegrees(float rad)
{
    return rad * 180.f / M_PI;
}

/******************************************************************************/

float coneRadiusAtDistance(Cone cone, float distance)
{
    const float angle  = cone.angle * 0.5f;
    return distance * tan(degreesToRadians(angle));
}

/******************************************************************************/

vec3 uniformHemisphereSampling(float ux, float uy)
{
    clamp(ux, 0.f, 1.f);
    clamp(uy, 0.f, 1.f);
    const float r = sqrt(1.f - ux * ux);
    const float phi = 2 * M_PI * uy;
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
        onb.S.z = 0;
    } else {
        onb.S.x = 0;
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

    return imageLoad(octreeBrickTex, getBrickCoord(currentNode));
}

/******************************************************************************/

vec3 calculateDiffuseColorAO(const vec3 normal, const vec3 pos)
{
    vec4 totalColor = vec4(0);
    const float step = (1.f / float(u_coneGridSize));
    float occlusion = 0.0f;

    for (uint y = 0; y < u_coneGridSize; ++y) {

        const float uy = (0.5f + float(y)) * step;

        for (uint x = 0; x < u_coneGridSize; ++x) {

            const float ux = (0.5f + float(x)) * step;

            // create the cone
            ONB onb = toONB(normal);
            vec3 v = uniformHemisphereSampling(ux, uy); //  do random here if you want
            Cone cone;
            cone.dir   = normalize(toWorld(onb, v));
            cone.angle = 180.f / float(u_coneGridSize);

            // calculate weight
            float d = abs(dot(normalize(normal), cone.dir));

            // trace the cone for each sample
            for (uint step = 1; step <= u_numSteps; ++step) {

                const float totalDist = step * voxelSize;
                const float diameter = 2 * coneRadiusAtDistance(cone, totalDist);
                if (diameter <= 0.f) continue; // some error (angle < 0 || angle > 90)

                // calculate mipmap level
                int level = int(u_treeLevels);
                float voxel_size = voxelSize;
                while (voxel_size < diameter && level > 0) {
                    voxel_size *= 2;
                    --level;
                }

                // get indirect color
                const vec3 wpos = pos + totalDist * cone.dir;
                vec4 color = getColor(level, wpos);

                // maybe we are occluded
                if(color.w == 0.0)
                {
                    occlusion += 1.f * d;
                    break;
                }
                else
                {
                    // amplify the color
                    color.xyz *= d*d;
                    color /= (step * step * voxelSize * voxelSize);
                    totalColor += color;
                }

                // if the color is "filled up", break
                if (totalColor.w >= 1.0) {
                    break;
                }

            }

        }

    }

    if (totalColor.w > 0) {
        totalColor /= totalColor.w;
    }

    // ambient occlusion - correct? incorrect? -1? no -1?
    occlusion = 1.f - clamp(occlusion / float(u_coneGridSize * u_coneGridSize), 0.f, 1.f);

    return totalColor.xyz * occlusion;
}

/******************************************************************************/

vec3 traceCone(in vec3 origin, in vec3 direction, in float angle, in int steps)
{
    const float tan_a = tan(angle / 2.0);

    vec3 result = vec3(0.0);
    float alpha = 1.0;
    float diameter = voxelSize;
    float dist = 0.0;
    for (int i = 0; i < steps; ++i) {
        dist += diameter;
        diameter = 2.0 * (tan_a * dist);
        vec3 pos = origin + dist * direction;

        // calculate mipmap level
        uint level = min(uint(log2(diameter / voxelSize)), u_treeLevels - 1);

        // get indirect color
        vec4 color = getColor(level, pos);


        result += alpha * color.a * color.rgb;

        if(color.a == 0.0) {
            break;
        }

        alpha *= (1.0 - color.a);
    }

    return result;
}

/******************************************************************************/

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


    float angle = degreesToRadians(20.0) * glossy;
    vec3 spec = specular * traceCone(wpos.xyz, refl, angle, int(u_numSteps));

    outFragColor = vec4(spec + 2.5 * calculateDiffuseColorAO(normal, wpos.xyz), 1.0);
}

/******************************************************************************/
