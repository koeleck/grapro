#version 440 core

#include "common/camera.glsl"

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
out vec3 vs_tangent;
out vec3 vs_bitangent;
#endif
out vec3 vs_viewdir;

uniform mat4 uModelMatrix;

void main()
{
    vec4 worldPos = uModelMatrix * vec4(in_position, 1.0);
    vs_viewdir = normalize(cam.Position.xyz - worldPos.xyz);
#ifdef HAS_NORMALS
    vs_normal = (uModelMatrix * vec4(in_normal, 0.0)).xyz;
#endif
#ifdef HAS_TEXCOORDS
    vs_uv = in_uv;
#endif
#ifdef HAS_TANGENTS
    vs_tangent = (uModelMatrix * vec4(in_tangent, 0.0)).xyz;
    vs_bitangent = normalize(cross(vs_normal, vs_tangent));
#endif
    gl_Position = cam.ProjViewMatrix * worldPos;
}
