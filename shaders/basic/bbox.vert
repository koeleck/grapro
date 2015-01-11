#version 440 core

#include "common/extensions.glsl"
#include "common/camera.glsl"
#include "common/instances.glsl"

void main()
{
    const uint instanceID = gl_BaseInstanceARB + gl_InstanceID;
    vec3 BBox[2] = vec3[2](instances[instanceID].bbox_min.xyz,
                           instances[instanceID].bbox_max.xyz);

    vec3 pmin = vec3(1.0);
    vec3 pmax = vec3(0.0);
    for (uint i = 0; i < 8; ++i) {
        vec4 pos = vec4(BBox[(i>>0) & 0x01].x,
                        BBox[(i>>1) & 0x01].y,
                        BBox[(i>>2) & 0x01].z,
                        1.0);
        pos = cam.ProjViewMatrix * pos;
        vec3 tmp = (pos.xyz / pos.w) * 0.5 + 0.5;
        pmin = min(pmin, tmp);
        pmax = max(pmax, tmp);
    }

    //vec4 pos = vec4(BBox[(gl_VertexID>>0) & 0x01].x,
    //                BBox[(gl_VertexID>>1) & 0x01].y,
    //                BBox[(gl_VertexID>>2) & 0x01].z,
    //                1.0);

    //gl_Position = cam.ProjViewMatrix * pos;
}
