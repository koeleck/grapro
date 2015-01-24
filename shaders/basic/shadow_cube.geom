#version 440

#include "common/lights.glsl"
#include "common/bindings.glsl"

layout(triangles, invocations = NUM_SHADOWCUBEMAPS) in;
layout(triangle_strip, max_vertices = 18) out; // 6 sides x 3 vertices

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


void emitTriangle(in int ID, in vec4 vertex[3], in int layer)
{
    // cull back faces
    vec3 normal = cross(vertex[1].xyz - vertex[0].xyz,
                        vertex[2].xyz - vertex[0].xyz);
    if (normal.z < 0.0)
        return;

    // frustum culling
    const mat4 ProjMat = lights[ID].ProjViewMatrix; // this is actually just a projection matrix
    int outOfBound[6] = int[6](0, 0, 0, 0, 0, 0);
    for (int i = 0; i < 3; ++i) {
        vertex[i] = ProjMat * vertex[i];
        if (vertex[i].x >  vertex[i].w) ++outOfBound[0];
        if (vertex[i].x < -vertex[i].w) ++outOfBound[1];
        if (vertex[i].y >  vertex[i].w) ++outOfBound[2];
        if (vertex[i].y < -vertex[i].w) ++outOfBound[3];
        if (vertex[i].z >  vertex[i].w) ++outOfBound[4];
        if (vertex[i].z < -vertex[i].w) ++outOfBound[5];
    }
    for (int i = 0; i < 6; ++i) {
        if (outOfBound[i] == 3)
            return;
    }

    for (int i = 0; i < 3; ++i) {
        gl_Layer = layer;
        gl_Position = vertex[i];
        gl_Position.z += BIAS;
        outData.uv = inData[i].uv;
        outData.materialID = inData[i].materialID;
        EmitVertex();
    }
    EndPrimitive();
}

/* For reference: indices of cube sides
 0: GL_TEXTURE_CUBE_MAP_POSITIVE_X
 1: GL_TEXTURE_CUBE_MAP_NEGATIVE_X
 2: GL_TEXTURE_CUBE_MAP_POSITIVE_Y
 3: GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
 4: GL_TEXTURE_CUBE_MAP_POSITIVE_Z
 5: GL_TEXTURE_CUBE_MAP_NEGATIVE_Z

 ProjMatrix looks along the negativ z-axis, so
 rotate vertices accordingly:
*/

// transposed rotation matrices
const mat3 RotationMatrix[6] = mat3[6](

    mat3(vec3( 0.0,  0.0, -1.0),
         vec3( 0.0, -1.0,  0.0),
         vec3(-1.0,  0.0,  0.0)),

    mat3(vec3( 0.0,  0.0,  1.0),
         vec3( 0.0, -1.0,  0.0),
         vec3( 1.0,  0.0,  0.0)),

    mat3(vec3( 1.0,  0.0,  0.0),
         vec3( 0.0,  0.0,  1.0),
         vec3( 0.0, -1.0,  0.0)),
    mat3(vec3( 1.0,  0.0,  0.0),
         vec3( 0.0,  0.0, -1.0),
         vec3( 0.0,  1.0,  0.0)),

    mat3(vec3( 1.0,  0.0,  0.0),
         vec3( 0.0, -1.0,  0.0),
         vec3( 0.0,  0.0, -1.0)),

    mat3(vec3(-1.0,  0.0,  0.0),
         vec3( 0.0, -1.0,  0.0),
         vec3( 0.0,  0.0,  1.0))
);


void main()
{
    const int ID = lightID[gl_InvocationID];
    if (ID == -1)
        return;

    const int baseLayer = 6 * (lights[ID].type_texid & LIGHT_TEXID_BITS);
    const vec3 pos = lights[ID].position;

    vec3 baseVertex[3] = vec3[3](gl_in[0].gl_Position.xyz - pos,
                                 gl_in[1].gl_Position.xyz - pos,
                                 gl_in[2].gl_Position.xyz - pos);

    for (int i = 0; i < 6; ++i) {
        vec4 vertex[3] = vec4[3](
                vec4(baseVertex[0] * RotationMatrix[i], 1.0),
                vec4(baseVertex[1] * RotationMatrix[i], 1.0),
                vec4(baseVertex[2] * RotationMatrix[i], 1.0));
        emitTriangle(ID, vertex, baseLayer + i);
    }
}
