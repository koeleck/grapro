#version 440 core


uniform sampler2D u_pos;
uniform sampler2D u_normal;
uniform sampler2D u_color;
uniform sampler2D u_depth;

layout(location = 0) out vec4 out_color;

const float M_PI                  = 3.14159265359;
const unsigned int num_sqrt_cones = 2; // 2x2 grid        

struct Cone
{
    vec3 pos;
    vec3 dir;
    float angle;
} cone[num_sqrt_cones*num_sqrt_cones];

vec3 UniformHemisphereSampling(float ux, float uy)
{
    const float r = sqrt(1.f - ux * ux);
    const float phi = 2 * M_PI * uy;
    return vec3(cos(phi) * r, sin(phi) * r, ux);
}

void main()
{
    vec4 pos    = texture(u_pos, gl_FragCoord.xy);
    vec3 normal = texture(u_normal, gl_FragCoord.xy).xyz;
    float depth = texture(u_color, gl_FragCoord.xy).x;
    vec3 color  = texture(u_depth, gl_FragCoord.xy).xyz;

    // cone tracing
    float ux = 0.f;
    float uy = 0.f;
    for(unsigned int y = 0; y < num_sqrt_cones; ++y)
    {
        uy = y * (1.f / float(num_sqrt_cones));
        for(unsigned int x = 0; x < num_sqrt_cones; ++x)
        {
            ux = x * (1.f / float(num_sqrt_cones));

            cone[y * num_sqrt_cones + x].pos   = pos.xyz;
            cone[y * num_sqrt_cones + x].dir   = UniformHemisphereSampling(ux, uy);
            cone[y * num_sqrt_cones + x].angle = 180 / num_sqrt_cones;
        }
    }


    out_color = vec4(1, normal.x, normal.y, 1);
}
