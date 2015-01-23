#version 440

#include "voxel.glsl"

layout(location = 0) out vec4 out_FragColor;

flat in uint ID;

void main()
{
    vec3 col = octreeColor[ID].diffuse.rgb;
    col /= octreeColor[ID].count;

    out_FragColor = vec4(col, 1.0);
    //out_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}

