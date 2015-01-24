#version 440 core

#include "common/extensions.glsl"
#include "common/voxel.glsl"
#include "common/camera.glsl"

layout(location = 0) out vec4 out_color;

/******************************************************************************/

uniform uint u_voxelDim;
uniform vec3 u_bboxMin;
uniform vec3 u_bboxMax;
uniform uint u_screenwidth;
uniform uint u_screenheight;
uniform uint u_treeLevels;
uniform uint u_diffuseConeGridSize;
uniform uint u_diffuseConeSteps;
uniform uint u_specularConeSteps;
uniform uint u_aoConeGridSize;
uniform uint u_aoConeSteps;
uniform uint u_aoWeight;

uniform sampler2D u_pos;
uniform sampler2D u_normal;
uniform sampler2D u_color;

/******************************************************************************/

const float voxelSize = (u_bboxMax.x - u_bboxMin.x) / float(u_voxelDim);

/******************************************************************************/

vec4 getColor(uint maxlevel, vec3 wpos)
{
    const ivec3 pos = ivec3((wpos - u_bboxMin) / voxelSize);
    uint childIdx = 0;
    uint nodePtr = octree[childIdx].id;
    int voxelDim = int(u_voxelDim);
    ivec3 umin = ivec3(0);

    for (uint i = 0; i < maxlevel - 1; ++i) {

        if ((nodePtr & 0x80000000) == 0) {
            // no flag set -> no child nodes
            return vec4(0);
        }

        iterateTreeLevel(pos, nodePtr, voxelDim, childIdx, umin);

    }

    vec4 col = octreeColor[childIdx].color;
    if (col.w == 0.f) return vec4(0);
    col /= col.w;
    return col;
}

/******************************************************************************/

bool isOccluded(uint maxlevel, vec3 wpos)
{

    const ivec3 pos = ivec3((wpos - u_bboxMin) / voxelSize);
    uint childIdx = 0;
    uint nodePtr = octree[childIdx].id;
    int voxelDim = int(u_voxelDim);
    ivec3 umin = ivec3(0);

    // iterate through all tree levels
    for (uint i = 0; i < maxlevel - 1; ++i) {

        // check occlusion
        if((nodePtr & 0x80000000) == 0) {
            // no flag set -> no child nodes
            return false;
        }

        iterateTreeLevel(pos, nodePtr, voxelDim, childIdx, umin);

    }

    vec4 col = octreeColor[childIdx].color;
    if (col.w == 0.f) return false;
    return true;

}

/******************************************************************************/

vec4 getNormal(uint maxlevel, vec3 wpos)
{
    const ivec3 pos = ivec3((wpos - u_bboxMin) / voxelSize);
    uint childIdx = 0;
    uint nodePtr = octree[childIdx].id;
    int voxelDim = int(u_voxelDim);
    ivec3 umin = ivec3(0);

    for (uint i = 0; i < maxlevel - 1; ++i) {

        if ((nodePtr & 0x80000000) == 0) {
            // no flag set -> no child nodes
            return vec4(0);
        }

        iterateTreeLevel(pos, nodePtr, voxelDim, childIdx, umin);

    }

    vec4 normal = octreeColor[childIdx].normal;
    if (normal.w == 0.f) return vec4(0);
    normal /= normal.w;
    return normal;
}

/******************************************************************************/

vec3 calculateDiffuseColor(const vec3 normal, const vec3 pos)
{
	vec4 totalColor = vec4(0);
	const float step = (1.f / float(u_diffuseConeGridSize));

	const float theta = cos(30 * int(gl_FragCoord.x) ^ int(gl_FragCoord.y) + 10 * int(gl_FragCoord.x) * int(gl_FragCoord.y));
	const float cs = cos(theta);
	const float sn = sin(theta);

    for (uint y = 0; y < u_diffuseConeGridSize; ++y) {

        const float uy = (0.5f + float(y)) * step;

        for (uint x = 0; x < u_diffuseConeGridSize; ++x) {

            const float ux = (0.5f + float(x)) * step;

            const float uxRot = ux * cs - uy * sn;
            const float uyRot = ux * sn + uy * cs;

            // create the cone
            ONB onb = toONB(normal);
            vec3 v = uniformHemisphereSampling(ux, uy); //  do random here if you want
            Cone cone;
            cone.dir   = normalize(toWorld(onb, v));
            cone.angle = 180.f / float(u_diffuseConeGridSize);

            // calculate weight
            float d = abs(dot(normalize(normal), cone.dir));

            // trace the cone for each sample
            for (uint step = 1; step <= u_diffuseConeSteps; ++step) {

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
                const vec3 normal = getNormal(level, wpos).xyz;
                if (dot(normal, cone.dir) > 0.f) {
                    //continue;
                }
                vec4 color = getColor(level, wpos);
                if (color.w > 0) {
                    // color found -> stop walking!
                    color.xyz *= d*d;
                    color /= (step * step * voxelSize * voxelSize);
                    totalColor += color;
                    break;
                }

            }

        }

    }

    if (totalColor.w > 0) {
        totalColor /= totalColor.w;
    }

    return totalColor.xyz;
}

/******************************************************************************/

vec3 calculateSpecularColor(const vec3 normal, const vec3 pos)
{
	const vec3 viewDir = normalize(pos - cam.Position.xyz);
	const vec3 reflectVec = normalize(reflect(viewDir, normalize(normal)));

	Cone cone;
    cone.dir = reflectVec;
    cone.angle = 20.f; //  TODO

    vec4 totalColor = vec4(0);
    float actualVoxelSize = voxelSize;

    // trace the cone for each sample
    for (uint step = 1; step <= u_specularConeSteps; ++step) {

        const float totalDist = step * actualVoxelSize;
        const float diameter = 2 * coneRadiusAtDistance(cone, totalDist);
        if (diameter <= 0.f) continue; // some error (angle < 0 || angle > 90)

        // calculate mipmap level
        int level = int(u_treeLevels);
        float voxel_size = voxelSize;
        while (voxel_size < diameter && level > 0) {
            voxel_size *= 2;
            --level;
        }
        actualVoxelSize = voxel_size;

        // get indirect color
        const vec3 wpos = pos + totalDist * cone.dir;
        const vec3 normal = getNormal(level, wpos).xyz;
        if (dot(normal, cone.dir) > 0.f) {
            continue;
        }
        const vec4 color = getColor(level, wpos);
        if (color.w > 0) {
            // color found -> stop walking!;
            totalColor = color / color.w;
            totalColor /= (step);
            break;
        }

    }

    return totalColor.xyz;
}

float calculateAmbientOcclusion(const vec3 normal, const vec3 pos)
{
    float occlusion = 0.f;
    const float step = (1.f / float(u_aoConeGridSize));

    for (uint y = 0; y < u_aoConeGridSize; ++y) {

        const float uy = (0.5f + float(y)) * step;

        for (uint x = 0; x < u_aoConeGridSize; ++x) {

            const float ux = (0.5f + float(x)) * step;

            // create the cone
            ONB onb = toONB(normal);
            vec3 v = uniformHemisphereSampling(ux, uy); //  do random here if you want
            Cone cone;
            cone.dir   = normalize(toWorld(onb, v));
            cone.angle = 180.f / float(u_aoConeGridSize);

            // calculate weight
            float d = abs(dot(normalize(normal), cone.dir));

            // trace the cone for each sample
            for (uint step = 1; step <= u_aoConeSteps; ++step) {

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

                // ambient occlusion
                const vec3 wpos = pos + totalDist * cone.dir;
                vec4 color = getColor(level, wpos);
                if (isOccluded(level, wpos)) {
                    // we are occluded here
                    occlusion += 1.f * pow(d, float(u_aoWeight));
                    break;
                }

            }

        }

    }

    return 1.f - clamp(occlusion / float(u_aoConeGridSize * u_aoConeGridSize), 0.f, 1.f);;
}

/******************************************************************************/

void main()
{
    const vec2 uv = gl_FragCoord.xy / vec2(u_screenwidth, u_screenheight);
    const vec3 normal = texture(u_normal, uv).xyz;
    if (normal == 0) return;
    const vec3 pos = texture(u_pos, uv).xyz;
    const vec3 color = texture(u_color, uv).xyz;

    vec3 diffuse = calculateDiffuseColor(normal, pos);
    //vec3 specular = calculateSpecularColor(normal, pos);
    float occlusion = calculateAmbientOcclusion(normal, pos);

    //out_color = vec4(mix(mix(color, diffuse, 0.2), specular, 0.01), 1);
    vec3 diffuseMix = mix(color, diffuse, 0.4);
    out_color = vec4(max(diffuseMix, diffuse) * occlusion, 1);
    //out_color = vec4(occlusion, 1);
}

/******************************************************************************/
