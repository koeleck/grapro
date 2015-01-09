#version 440 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

flat out vec3 dominantAxis;
flat out vec4 f_AABB;
flat out float test;

uniform mat4 u_MVPx;
uniform mat4 u_MVPy;
uniform mat4 u_MVPz;

uniform int u_width;
uniform int u_height;

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
	//a = proj * a;
	//b = proj * b;
	//c = proj * c;

	//Next we enlarge the triangle to enable conservative rasterization
	vec4 AABB;
	vec2 hPixel = vec2(1.f / u_width, 1.f / u_height);

	//calculate AABB of this triangle
	AABB.xy = a.xy;
	AABB.zw = a.xy;
	AABB.xy = min(b.xy, AABB.xy);
	AABB.zw = max(b.xy, AABB.zw);
	AABB.xy = min(c.xy, AABB.xy);
	AABB.zw = max(c.xy, AABB.zw);
	//Enlarge half-pixel
	AABB.xy -= hPixel;
	AABB.zw += hPixel;
	f_AABB = AABB;

	//find 3 triangle edge plane
	vec3 e0 = vec3(b.xy - a.xy, 0.f);
	vec3 e1 = vec3(c.xy - b.xy, 0.f);
	vec3 e2 = vec3(a.xy - c.xy, 0.f);
	vec3 n0 = cross(e0, vec3(0.f, 0.f, 1.f));
	vec3 n1 = cross(e1, vec3(0.f, 0.f, 1.f));
	vec3 n2 = cross(e2, vec3(0.f, 0.f, 1.f));
	//dilate the triangle
	const float pl = sqrt(hPixel[0] * hPixel[0] + hPixel[1] * hPixel[1]);
	a.xy = a.xy + pl * ((e2.xy / dot(e2.xy, n0.xy)) + (e0.xy / dot(e0.xy, n2.xy)));
	b.xy = b.xy + pl * ((e0.xy / dot(e0.xy, n1.xy)) + (e1.xy / dot(e1.xy, n0.xy)));
	c.xy = c.xy + pl * ((e1.xy / dot(e1.xy, n2.xy)) + (e2.xy / dot(e2.xy, n1.xy)));

	gl_Position = a;
	EmitVertex();

	gl_Position = b;
	EmitVertex();

	gl_Position = c;
	EmitVertex();

}
