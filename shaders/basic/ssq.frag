#version 440

layout(location = 0) out vec4 outFragColor;

in vec2 vsTexCoord;

layout(binding = 0) uniform sampler2DArray uTexture;
//layout(binding = 0) uniform samplerCubeArray uTexture;

void main()
{
    vec3 texcoord = vec3(vsTexCoord, 0.0);
    float val = texture(uTexture, texcoord).r;

    //vec3 dir = vec3(1.0, -1.0, 1.0);
    //dir.yz += 2.0 * vsTexCoord.xy;

    //vec4 texcoord = vec4(dir, 0.0);

    //float val = texture(uTexture, texcoord).r;
    //float zn = 25.0;
    //float zf = 2500.0;
    //float lin_dist = - (zn*zf) / (val * (zf - zn) - zf);
    //outFragColor = vec4(vec3((lin_dist - zn) / (zf - zn)), 1.0);
    outFragColor = vec4(vec3(val), 1.0);
}
