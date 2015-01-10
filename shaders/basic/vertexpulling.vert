#version 440 core

#include "common/extensions.glsl"
#include "common/camera.glsl"
#include "common/instances.glsl"
#include "common/meshes.glsl"
#include "common/vertices.glsl"

out VertexData
{
    vec3 viewdir;
    vec3 normal;
    vec2 uv;
    vec3 tangent;
    vec3 bitangent;
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

    const vec4 worldPos = modelMatrix* vec4(value, 1.0);

    outData.viewdir = normalize(cam.Position.xyz - worldPos.xyz);

    if ((components & MESH_COMPONENT_TEXCOORD) != 0) {
        value.x = vertexData[idx++];
        value.y = vertexData[idx++];
        outData.uv = value.xy;
    }
    if ((components & MESH_COMPONENT_NORMAL) != 0) {
        value.x = vertexData[idx++];
        value.y = vertexData[idx++];
        value.z = vertexData[idx++];
        outData.normal = normalize((modelMatrix * vec4(value, 0.0)).xyz);
    }
    if ((components & MESH_COMPONENT_TANGENT) != 0) {
        value.x = vertexData[idx++];
        value.y = vertexData[idx++];
        value.z = vertexData[idx++];
        outData.tangent = normalize((modelMatrix * vec4(value, 0.0)).xyz);
        value.x = vertexData[idx++];
        value.y = vertexData[idx++];
        value.z = vertexData[idx++];
        outData.bitangent = normalize((modelMatrix * vec4(value, 0.0)).xyz);
        const float reflect_normal = vertexData[idx++];
        outData.normal = reflect_normal * normalize(cross(outData.tangent, outData.bitangent));
    }
    gl_Position = cam.ProjViewMatrix * worldPos;
}
