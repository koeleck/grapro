#version 440

#include "common/lights.glsl"
#include "common/bindings.glsl"

layout(triangles, invocations = NUM_SHADOWMAPS) in;
layout(triangle_strip, max_vertices = 3) out;

in VertexData
{
    vec2 uv;
    flat uint materialID;
} inData[];

out VertexData
{
    vec2 uv;
    flat uint materialID;
} outData;


layout(std430, binding = LIGHT_ID_BINDING) restrict readonly buffer LightIDBlock
{
    int     lightID[];
};

// TODO
const float BIAS = 0.2;

void main()
{
    const int ID = lightID[gl_InvocationID];
    if (ID == -1)
        return;

    const mat4 PVM = lights[ID].ProjViewMatrix;
    const int layer = lights[ID].type_texid & LIGHT_TEXID_BITS;

    // cull front faces
    vec3 normal = cross(gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz,
                        gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz);
    vec3 view = lights[ID].position - gl_in[0].gl_Position.xyz;
    if (dot(normal, view) > 0.0)
        return;

    // frustum culling
    vec4 vertex[3];
    int outOfBound[6] = int[6](0, 0, 0, 0, 0, 0);
    for (int i = 0; i < 3; ++i) {
        vertex[i] = PVM * gl_in[i].gl_Position;
        vertex[i].z += BIAS;
        if (vertex[i].x > +vertex[i].w) ++outOfBound[0];
        if (vertex[i].x < -vertex[i].w) ++outOfBound[1];
        if (vertex[i].y > +vertex[i].w) ++outOfBound[2];
        if (vertex[i].y < -vertex[i].w) ++outOfBound[3];
        if (vertex[i].z > +vertex[i].w) ++outOfBound[4];
        if (vertex[i].z < -vertex[i].w) ++outOfBound[5];
    }
    for (int i = 0; i < 6; ++i) {
        if (outOfBound[i] == 3)
            return;
    }

    for (int i = 0; i < 3; ++i) {
        gl_Layer = layer;
        gl_Position = vertex[i];
        outData.uv = inData[i].uv;
        outData.materialID = inData[i].materialID;
        EmitVertex();
    }
    EndPrimitive();
}
