#version 440

layout(location = 0) out vec4 outFragColor;

in vec2 vsTexCoord;

layout(location = 0) uniform sampler2DArray uTexture;

void main()
{
    vec3 texcoord = vec3(vsTexCoord, 1.0);
    //outFragColor = vec4(texture(uTexture, texcoord).rgb, 1.0);

    float val = texture(uTexture, texcoord).r;
    float zn = 0.1;
    float zf = 5000.0;
    float lin_dist = - (zn*zf) / (val * (zf - zn) - zf);
    outFragColor = vec4(vec3((lin_dist - zn) / (zf - zn)), 1.0);
}
