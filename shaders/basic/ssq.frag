#version 440

#include "common/compression.glsl"

layout(location = 0) out vec4 outFragColor;

in vec2 vsTexCoord;

layout(binding = 0) uniform sampler2D uTexture;

/*
const float THRESH=30./255.;

float edge_filter(in vec2 center, in vec2 a0, in vec2 a1,
                  in vec2 a2, in vec2 a3)
{
    vec4 lum = vec4(a0.x, a1.x , a2.x, a3.x);
    vec4 w = 1.0 - step(THRESH, abs(lum - center.x));
    float W = w.x + w.y + w.z + w.w;
    //Handle the special case where all the weights are zero.
    //In HDR scenes it's better to set the chrominance to zero.
    //Here we just use the chrominance of the first neighbor.
    w.x = (W == 0.0) ? 1.0 : w.x;
    W = (W == 0.0) ? 1.0 : W;

    return (w.x * a0.y + w.y * a1.y + w.z * a2.y +w.w * a3.y) / W;
    //W = (W == 0.0) ? W : 1.0 / W;
    //return (w.x * a0.y + w.y * a1.y + w.z * a2.y + w.w * a3.y) * W;
}
*/

void main()
{
    ivec2 crd = ivec2(gl_FragCoord.xy);
    /*
    bool isBlack = ((crd.x & 1) == (crd.y & 1));
    */

    vec4 col = texelFetch(uTexture, crd, 0);
    outFragColor = vec4(col.rrr, 1.0);


    /* nearest */
    /*
    float chroma = texelFetchOffset(uTexture, crd, 0, ivec2(1, 0)).g;
    */

    /* bilinear */
    /*
    float chroma = 0.25 * (
            texelFetchOffset(uTexture, crd, 0, ivec2( 1,  0)).g +
            texelFetchOffset(uTexture, crd, 0, ivec2(-1,  0)).g +
            texelFetchOffset(uTexture, crd, 0, ivec2( 0,  1)).g +
            texelFetchOffset(uTexture, crd, 0, ivec2( 0, -1)).g);
    */

    /* edge directed */
    /*
    vec2 a0 = texelFetchOffset(uTexture, crd, 0, ivec2( 1,  0)).rg;
    vec2 a1 = texelFetchOffset(uTexture, crd, 0, ivec2(-1,  0)).rg;
    vec2 a2 = texelFetchOffset(uTexture, crd, 0, ivec2( 0,  1)).rg;
    vec2 a3 = texelFetchOffset(uTexture, crd, 0, ivec2( 0, -1)).rg;
    float chroma = edge_filter(col.rg, a0, a1, a2, a3);


    col.b = chroma;
    col.rgb = (isBlack) ? col.rgb : col.rbg;
    col.rgb = YCoCg2RGB(col.rgb);

    outFragColor = vec4(col.rgb, 1.0);
    */
}
