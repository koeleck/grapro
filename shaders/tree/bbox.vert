#version 440 core

#include "common/extensions.glsl"
#include "common/camera.glsl"
#include "voxel.glsl"

layout(location = 0) uniform float uHalfSize;

flat out ivec3 vsBrickCoord;;

void main()
{
    uint ID = gl_BaseInstanceARB + gl_InstanceID;
    vsBrickCoord = getBrickCoord(ID);
    vec3 bbox[2];
    if ((octree[ID].id & 0x80000000u) == 0) {
        bbox[0] = vec3(-1000.0);
        bbox[1] = vec3(-1000.0);
    } else {
        vec3 midpoint = octreeInfo[ID].position;
        bbox[0] = midpoint - uHalfSize;
        bbox[1] = midpoint + uHalfSize;
    }

    vec4 pos = vec4(bbox[(gl_VertexID>>0) & 0x01].x,
                    bbox[(gl_VertexID>>1) & 0x01].y,
                    bbox[(gl_VertexID>>2) & 0x01].z,
                    1.0);

    //vsBrickCoord += ivec3(-1 + 2 * ((gl_VertexID>>0) & 0x01),
    //                      -1 + 2 * ((gl_VertexID>>1) & 0x01),
    //                      -1 + 2 * ((gl_VertexID>>2) & 0x01));

    gl_Position = cam.ProjViewMatrix * pos;
}
