#version 440

#include "voxel.glsl"

layout(location = 0) out vec4 out_FragColor;

flat in ivec3 vsBrickCoord;

layout(location = 1) uniform uint uColored;

void main()
{
    if (uColored != 0) {
    	vec4 res = imageLoad(octreeBrickTex, vsBrickCoord);
    	out_FragColor = vec4(res.xyz, 1.0);
    } else {
    	out_FragColor = vec4(1.0);
    }
}

