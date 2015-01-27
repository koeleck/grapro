#ifndef SHADERS_COMMON_BINDINGS_GLSL
#define SHADERS_COMMON_BINDINGS_GLSL

// Keep in sync with src/core/shader_interface.h

// Shader storage buffers
#define CAMERA_BINDING      0
#define TEXTURE_BINDING     1
#define MESH_BINDING        2
#define MATERIAL_BINDING    3
#define INSTANCE_BINDING    4
#define VERTEX_BINDING      5
#define VOXEL_BINDING       6
#define OCTREE_BINDING      7
#define LIGHT_BINDING       8
#define LIGHT_ID_BINDING    9
#define OCTREE_INFO_BINDING 10

#define DIFFUSE_TEX_UNIT        0
#define SPECULAR_TEX_UNIT       1
#define GLOSSY_TEX_UNIT         2
#define NORMAL_TEX_UNIT         3
#define EMISSIVE_TEX_UNIT       4
#define ALPHA_TEX_UNIT          5
#define AMBIENT_TEX_UNIT        6
#define SHADOWMAP_TEX_UNIT      7
#define SHADOWCUBEMAP_TEX_UNIT  8

#define GBUFFER_DEPTH_TEX                     9
#define GBUFFER_DIFFUSE_NORMAL_TEX            10
#define GBUFFER_SPECULAR_GLOSSY_EMISSIVE_TEX  11
#define BRICK_TEX                             12

#endif // SHADERS_COMMON_BINDINGS_GLSL
