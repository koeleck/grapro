#version 440 core

#include "common/camera.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 36) out;

uniform vec3 u_bboxMin;
uniform vec3 u_bboxMax;
uniform uint u_voxelDim;

in vec4 wpos[];
out vec4 worldPosition;

void main() {

	const float halfVoxelSize = (u_bboxMax.x - u_bboxMin.x) / float(u_voxelDim) * 0.5f;

    vec4 center = gl_in[0].gl_Position;
    vec4 a = center + vec4(-1,-1,-1,0) * halfVoxelSize;
    vec4 b = center + vec4(1,-1,-1,0) * halfVoxelSize;
    vec4 c = center + vec4(1,1,-1,0) * halfVoxelSize;
    vec4 d = center + vec4(-1,1,-1,0) * halfVoxelSize;
    vec4 e = center + vec4(-1,-1,1,0) * halfVoxelSize;
    vec4 f = center + vec4(1,-1,1,0) * halfVoxelSize;
    vec4 g = center + vec4(1,1,1,0) * halfVoxelSize;
    vec4 h = center + vec4(-1,1,1,0) * halfVoxelSize;

    // first side

    gl_Position = cam.ProjViewMatrix* a;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * b;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * c;
    worldPosition = wpos[0];
    EmitVertex();
    EndPrimitive();

    gl_Position = cam.ProjViewMatrix * a;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * c;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * d;
    worldPosition = wpos[0];
    EmitVertex();
    EndPrimitive();

    // second side

    gl_Position = cam.ProjViewMatrix * e;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * a;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * d;
    worldPosition = wpos[0];
    EmitVertex();
    EndPrimitive();

    gl_Position = cam.ProjViewMatrix * e;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * d;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * h;
    worldPosition = wpos[0];
    EmitVertex();
    EndPrimitive();

    // third side

    gl_Position = cam.ProjViewMatrix * f;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * e;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * h;
    worldPosition = wpos[0];
    EmitVertex();
    EndPrimitive();

    gl_Position = cam.ProjViewMatrix * f;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * h;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * g;
    worldPosition = wpos[0];
    EmitVertex();
    EndPrimitive();

    // fourth side

    gl_Position = cam.ProjViewMatrix * b;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * f;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * g;
    worldPosition = wpos[0];
    EmitVertex();
    EndPrimitive();

    gl_Position = cam.ProjViewMatrix * b;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * g;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * c;
    worldPosition = wpos[0];
    EmitVertex();
    EndPrimitive();

    // fifth side

    gl_Position = cam.ProjViewMatrix * d;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * c;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * g;
    worldPosition = wpos[0];
    EmitVertex();
    EndPrimitive();

    gl_Position = cam.ProjViewMatrix * d;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * g;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * h;
    worldPosition = wpos[0];
    EmitVertex();
    EndPrimitive();

    // sixth side

    gl_Position = cam.ProjViewMatrix * e;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * f;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * b;
    worldPosition = wpos[0];
    EmitVertex();
    EndPrimitive();

    gl_Position = cam.ProjViewMatrix * e;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * b;
    worldPosition = wpos[0];
    EmitVertex();
    gl_Position = cam.ProjViewMatrix * a;
    worldPosition = wpos[0];
    EmitVertex();
    EndPrimitive();

}
