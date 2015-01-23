#version 440 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

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

uniform int uNumVoxels;

const vec3 axes[3] = vec3[3](vec3(1.0, 0.0, 0.0),
                             vec3(0.0, 1.0, 0.0),
                             vec3(0.0, 0.0, 1.0));

void main() {

    vec4 a = gl_in[0].gl_Position;
    vec4 b = gl_in[1].gl_Position;
    vec4 c = gl_in[2].gl_Position;


    // calculate normal
    vec3 AC = (c - a).xyz;
    vec3 AB = (b - a).xyz;
    vec3 n = normalize(cross(AC, AB));

    // triangles are already in clip space [-1, 1]
    // and the local axis correspond to the world axis.
    // we choose a dominant axis and if necessary
    // swap some values. This will be reversed in the
    // fragment shader
    float axisID = 2;
    float d0 = abs(dot(n, axes[0]));
    float d1 = abs(dot(n, axes[1]));
    float d2 = abs(dot(n, axes[2]));
    if (d0 > d1 && d0 > d2) {
        // x dominant
        axisID = 0;
        // swap x & z
        vec3 tmp = vec3(a.x, b.x, c.x);
        a.x = a.z;
        a.z = tmp.x;
        b.x = b.z;
        b.z = tmp.y;
        c.x = c.z;
        c.z = tmp.z;

        // fix normal
        n = -n;
        tmp.x = n.x;
        n.x = n.z;
        n.z = tmp.x;
    } else if (d1 > d0 && d1 > d2) {
        // y dominant
        axisID = 1;
        // swap y & z
        vec3 tmp = vec3(a.y, b.y, c.y);
        a.y = a.z;
        a.z = tmp.x;
        b.y = b.z;
        b.z = tmp.y;
        c.y = c.z;
        c.z = tmp.z;

        // fix normal
        n = -n;
        tmp.x = n.y;
        n.y = n.z;
        n.z = tmp.x;
    }

    //Next we enlarge the triangle to enable conservative rasterization
    const float hPixel = 10.f / float(uNumVoxels);

    //calculate AABB of this triangle
    vec4 AABB = vec4(a.xy, a.xy);
    AABB.xy = min(b.xy, AABB.xy);
    AABB.xy = min(c.xy, AABB.xy);
    AABB.zw = max(b.xy, AABB.zw);
    AABB.zw = max(c.xy, AABB.zw);
    //Enlarge AABB by half-pixel
    AABB.xy -= hPixel;
    AABB.zw += hPixel;

    //find 3 triangle edge plane
    const vec3 e0 = normalize(vec3(b.xy - a.xy, 0.0));
    const vec3 e1 = normalize(vec3(c.xy - b.xy, 0.0));
    const vec3 e2 = normalize(vec3(a.xy - c.xy, 0.0));
    // make edge normals point outwards
    const float dir = sign(dot(n, axes[2]));
    const vec3 n0 = dir * cross(axes[2], e0);
    const vec3 n1 = dir * cross(axes[2], e1);
    const vec3 n2 = dir * cross(axes[2], e2);

    // dilate the triangle
    const float pl = sqrt(2.0) * hPixel;
    a.xy += pl * ((e2.xy / dot(e2.xy, n0.xy)) + (e0.xy / dot(e0.xy, n2.xy)));
    b.xy += pl * ((e0.xy / dot(e0.xy, n1.xy)) + (e1.xy / dot(e1.xy, n0.xy)));
    c.xy += pl * ((e1.xy / dot(e1.xy, n2.xy)) + (e2.xy / dot(e2.xy, n1.xy)));


    gl_Position = a;
    outData.AABB = AABB;
    outData.position = a.xyz;
    outData.axis = axisID;
    outData.normal = inData[0].normal;
    outData.tangent = inData[0].tangent;
    outData.bitangent = inData[0].bitangent;
    outData.materialID = inData[0].materialID;
    outData.uv = inData[0].uv;
    EmitVertex();

    gl_Position = b;
    outData.AABB = AABB;
    outData.position = b.xyz;
    outData.axis = axisID;
    outData.normal = inData[1].normal;
    outData.tangent = inData[1].tangent;
    outData.bitangent = inData[1].bitangent;
    outData.materialID = inData[1].materialID;
    outData.uv = inData[1].uv;
    EmitVertex();

    gl_Position = c;
    outData.AABB = AABB;
    outData.position = c.xyz;
    outData.axis = axisID;
    outData.normal = inData[2].normal;
    outData.tangent = inData[2].tangent;
    outData.bitangent = inData[2].bitangent;
    outData.materialID = inData[2].materialID;
    outData.uv = inData[2].uv;
    EmitVertex();
}
