#version 440 core

#include "common/extensions.glsl"
#include "common/camera.glsl"
#include "voxel.glsl"

layout(location = 0) uniform float uHalfSize;

flat out uint ID;

void main()
{
    ID = gl_BaseInstanceARB + gl_InstanceID;
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

    gl_Position = cam.ProjViewMatrix * pos;
}
