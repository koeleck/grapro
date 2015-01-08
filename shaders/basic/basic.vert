#version 440 core

#include "common/extensions.glsl"
#include "common/camera.glsl"
#include "common/instances.glsl"

layout(location = 0) in vec3 in_position;
#ifdef HAS_NORMALS
layout(location = 1) in vec3 in_normal;
out vec3 vs_normal;
#endif
#ifdef HAS_TEXCOORDS
layout(location = 2) in vec2 in_uv;
out vec2 vs_uv;
#endif
#ifdef HAS_TANGENTS
layout(location = 3) in vec3 in_tangent;
layout(location = 4) in vec3 in_bitangent;
out vec3 vs_tangent;
out vec3 vs_bitangent;
#endif

out vec3 vs_viewdir;
flat out uint materialID;

void main()
{
    const uint instanceID = gl_BaseInstanceARB + gl_InstanceID;
    materialID = instances[instanceID].materialID;

    const mat4 modelMatrix = instances[instanceID].modelMatrix;
    const vec4 worldPos = modelMatrix* vec4(in_position, 1.0);

    vs_viewdir = normalize(cam.Position.xyz - worldPos.xyz);
#ifdef HAS_NORMALS
    vs_normal = normalize((modelMatrix * vec4(in_normal, 0.0)).xyz);
#endif
#ifdef HAS_TEXCOORDS
    vs_uv = in_uv;
#endif
#ifdef HAS_TANGENTS
    vs_tangent = normalize(modelMatrix * vec4(in_tangent.xyz, 0.0)).xyz);
    vs_bitangent = normalize(modelMatrix * vec4(in_bitangent.xyz, 0.0)).xyz);
#endif
    gl_Position = cam.ProjViewMatrix * worldPos;
}
