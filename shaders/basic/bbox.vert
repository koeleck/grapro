#version 440 core

#include "common/extensions.glsl"
#include "common/camera.glsl"
#include "common/instances.glsl"

void main()
{
    const uint instanceID = gl_BaseInstanceARB + gl_InstanceID;
    vec3 BBox[2] = vec3[2](instances[instanceID].bbox_min.xyz,
                           instances[instanceID].bbox_max.xyz);

    /*
     * World space bounding box
     */
    vec4 pos = vec4(BBox[(gl_VertexID>>0) & 0x01].x,
                    BBox[(gl_VertexID>>1) & 0x01].y,
                    BBox[(gl_VertexID>>2) & 0x01].z,
                    1.0);

    gl_Position = cam.ProjViewMatrix * pos;

    /*
     * Screen space bounding box:
     */
    /*
    vec3 BBox2[2] = vec3[2](vec3(2.0), vec3(-2.0));
    for (int i = 0; i < 8; ++i) {
        vec4 pos = vec4(BBox[(i>>0) & 0x01].x,
                        BBox[(i>>1) & 0x01].y,
                        BBox[(i>>2) & 0x01].z,
                        1.0);
        vec4 tmp = cam.ProjViewMatrix * pos;
        tmp.xyz /= tmp.w;
        BBox2[0] = min(BBox2[0], tmp.xyz);
        BBox2[1] = max(BBox2[1], tmp.xyz);
    }
    gl_Position = vec4(BBox2[(gl_VertexID>>0) & 0x01].x,
                       BBox2[(gl_VertexID>>1) & 0x01].y,
                       BBox2[(gl_VertexID>>2) & 0x01].z,
                       1.0);
    */
}
