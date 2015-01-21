#version 440

#include "common/extensions.glsl"
#include "common/instances.glsl"
#include "common/meshes.glsl"
#include "common/vertices.glsl"

out VertexData
{
    vec2 uv;
    flat uint materialID;
} outData;

void main()
{
    const uint instanceID = gl_BaseInstanceARB + gl_InstanceID;
    const uint meshID = instances[instanceID].meshID;
    const uint components = meshes[meshID].components;
    const mat4 modelMatrix = instances[instanceID].modelMatrix;

    outData.materialID = instances[instanceID].materialID;

    uint idx = meshes[meshID].first + gl_VertexID * meshes[meshID].stride;

    vec3 value;
    value.x = vertexData[idx++];
    value.y = vertexData[idx++];
    value.z = vertexData[idx++];

    gl_Position = modelMatrix * vec4(value, 1.0);

    if ((components & MESH_COMPONENT_TEXCOORD) != 0) {
        value.x = vertexData[idx++];
        value.y = vertexData[idx++];
        outData.uv = value.xy;
    }
}

