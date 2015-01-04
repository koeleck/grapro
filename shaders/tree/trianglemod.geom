#version 440 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

out vec3 dominantAxis;

uniform mat4 u_MVPx;
uniform mat4 u_MVPy;
uniform mat4 u_MVPz;

const vec3 axes[3] = vec3[3](vec3(1.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f), vec3(0.f, 0.f, 1.f));

void main() {

	vec4 a = gl_in[0].gl_Position;
	vec4 b = gl_in[1].gl_Position;
	vec4 c = gl_in[2].gl_Position;

	// calculate normal
	vec3 AC = (c - a).xyz;
	vec3 AB = (b - a).xyz;
	vec3 n = cross(AC, AB);

	// choose dominantAxis & orthogonal projection
	dominantAxis = axes[0];
	mat4 proj = u_MVPx;
	float d0 = abs(dot(n, axes[0]));
	float d1 = abs(dot(n, axes[1]));
	float d2 = abs(dot(n, axes[2]));
	if (d1 > d0 && d1 > d2) {
		dominantAxis = axes[1];
		proj = u_MVPy;
	} else if (d2 > d0 && d2 > d1) {
		dominantAxis = axes[2];
		proj = u_MVPz;
	}

	// to clip space
	a = proj * a;
	b = proj * b;
	c = proj * c;

	gl_Position = a;
	EmitVertex();

	gl_Position = b;
	EmitVertex();

	gl_Position = c;
	EmitVertex();

}
