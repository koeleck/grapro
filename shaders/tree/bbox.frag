#version 440

#include "voxel.glsl"

layout(location = 0) out vec4 out_FragColor;

flat in ivec3 vsBrickCoord;
in vec3 vsTexCoords;

layout(location = 1) uniform uint uColored;
layout(location = 2) uniform bool uSmooth;

void main()
{
    if (uColored != 0) {
        vec4 res;
        if (uSmooth) {
            res = vec4(texture(uOctree3DTex, vsTexCoords).rgb, 1.0);
        } else {
            res = vec4(imageLoad(octreeBrickTex, vsBrickCoord).rgb, 1.0);
        }
        out_FragColor = res;
    } else {
        out_FragColor = vec4(1.0);
    }
}

