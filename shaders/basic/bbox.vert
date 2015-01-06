#version 440 core

#include "common/extensions.glsl"
#include "common/camera.glsl"
#include "common/instances.glsl"

void main()
{
    const uint instanceID = gl_BaseInstanceARB + gl_InstanceID;
    vec3 BBox[2] = vec3[2](instances[instanceID].bbox_min.xyz,
                           instances[instanceID].bbox_max.xyz);

    vec4 pos = vec4(BBox[(gl_VertexID>>0) & 0x01].x,
                    BBox[(gl_VertexID>>1) & 0x01].y,
                    BBox[(gl_VertexID>>2) & 0x01].z,
                    1.0);

    gl_Position = cam.ProjViewMatrix * pos;
}
