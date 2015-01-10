#version 440 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

flat in uint materialIDArr[];

in VertexData
{
    vec3 viewdir;
    vec3 normal;
    vec2 uv;
    vec3 tangent;
    vec3 bitangent;
    flat uint materialID;
} inData[];

out VertexFragmentData
{
    flat vec4 AABB;
    vec3 position;
    flat float axis;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    flat uint materialID;
    vec2 uv;
} outData;

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

    // triangles are already in clip space [-1, 1]
    // and the local axis correspond to the world axis.
    // we choose a dominant axis and if necessary
    // swap some values. This will be reversed in the
    // fragment shader
    vec3 dominantAxis = axes[2];
    float axis = 2;
    float d0 = abs(dot(n, axes[0]));
    float d1 = abs(dot(n, axes[1]));
    float d2 = abs(dot(n, axes[2]));
    if (d1 > d0 && d1 > d2) {
        // y dominant
        dominantAxis = axes[1];
        axis = 1;
        // swap y & z
        vec3 tmp = vec3(a.y, b.y, c.y);
        a.y = a.z;
        a.z = tmp.x;
        b.y = b.z;
        b.z = tmp.y;
        c.y = c.z;
        c.z = tmp.z;
    } else if (d0 > d1 && d0 > d2) {
        // x dominant
        dominantAxis = axes[0];
        axis = 0;
        // swap x & z
        vec3 tmp = vec3(a.x, b.x, c.x);
        a.x = a.z;
        a.z = tmp.x;
        b.x = b.z;
        b.z = tmp.y;
        c.x = c.z;
        c.z = tmp.z;
    }

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
    //Enlarge by half-pixel
    AABB.xy -= hPixel;
    AABB.zw += hPixel;

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
    outData.AABB = AABB;
    outData.position = a.xyz;
    outData.axis = axis;
    outData.normal = inData[0].normal;
    outData.tangent = inData[0].tangent;
    outData.bitangent = inData[0].bitangent;
    outData.materialID = inData[0].materialID;
    outData.uv = inData[0].uv;
    EmitVertex();

    gl_Position = b;
    outData.AABB = AABB;
    outData.position = b.xyz;
    outData.axis = axis;
    outData.normal = inData[1].normal;
    outData.tangent = inData[1].tangent;
    outData.bitangent = inData[1].bitangent;
    outData.materialID = inData[1].materialID;
    outData.uv = inData[1].uv;
    EmitVertex();

    gl_Position = c;
    outData.AABB = AABB;
    outData.position = c.xyz;
    outData.axis = axis;
    outData.normal = inData[2].normal;
    outData.tangent = inData[2].tangent;
    outData.bitangent = inData[2].bitangent;
    outData.materialID = inData[2].materialID;
    outData.uv = inData[2].uv;
    EmitVertex();

}
