#version 440 core

#include "common/extensions.glsl"
#include "common/camera.glsl"
#include "common/instances.glsl"
#include "common/meshes.glsl"
#include "common/vertices.glsl"

out vec3 vs_viewdir;
out vec3 vs_normal;
out vec2 vs_uv;
out vec3 vs_tangent;
out vec3 vs_bitangent;
flat out uint materialID;

void main()
{
    const uint instanceID = gl_BaseInstanceARB + gl_InstanceID;
    const uint meshID = instances[instanceID].meshID;
    const uint components = meshes[meshID].components;
    const mat4 modelMatrix = instances[instanceID].modelMatrix;

    materialID = instances[instanceID].materialID;

    uint idx = meshes[meshID].first + gl_VertexID * meshes[meshID].stride;

    vec3 value;
    value.x = vertexData[idx++];
    value.y = vertexData[idx++];
    value.z = vertexData[idx++];

    const vec4 worldPos = modelMatrix* vec4(value, 1.0);

    vs_viewdir = normalize(cam.Position.xyz - worldPos.xyz);

    if ((components & MESH_COMPONENT_NORMAL) != 0) {
        value.x = vertexData[idx++];
        value.y = vertexData[idx++];
        value.z = vertexData[idx++];
        vs_normal = normalize((modelMatrix * vec4(value, 0.0)).xyz);
    }
    if ((components & MESH_COMPONENT_TEXCOORD) != 0) {
        value.x = vertexData[idx++];
        value.y = vertexData[idx++];
        vs_uv = value.xy;
    }
    if ((components & MESH_COMPONENT_TANGENT) != 0) {
        value.x = vertexData[idx++];
        value.y = vertexData[idx++];
        value.z = vertexData[idx++];
        float sgn = vertexData[idx++];
        vs_tangent = normalize((modelMatrix * vec4(value, 0.0)).xyz);
        vs_bitangent = sgn * normalize(cross(vs_normal, vs_tangent));
    }
    gl_Position = cam.ProjViewMatrix * worldPos;
}
