#version 440

#include "voxel.glsl"

layout(location = 0) out vec4 out_FragColor;

flat in ivec3 vsBrickCoord;

void main()
{
    vec4 res = imageLoad(octreeBrickTex, vsBrickCoord);

    out_FragColor = vec4(res.rgb, 1.0);
    //out_FragColor = vec4(res.aaa, 1.0);
}

