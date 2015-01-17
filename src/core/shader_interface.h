#ifndef CORE_OPENGL_INTERFACE_H
#define CORE_OPENGL_INTERFACE_H

#include "gl/gl_sys.h"
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/vec2.hpp>
#include <glm/integer.hpp>

namespace core
{

/*****************************************************************************
 *
 * Binding points
 *
 ****************************************************************************/

namespace bindings
{

// Shader storage buffers:
constexpr int CAMERA    = 0;
constexpr int TEXTURE   = 1;
constexpr int MESH      = 2;
constexpr int MATERIAL  = 3;
constexpr int INSTANCE  = 4;
constexpr int VERTEX    = 5;
constexpr int VOXEL     = 6;
constexpr int OCTREE    = 7;
constexpr int LIGHT     = 8;

// Vertex Attrib Arrays
constexpr int POSITIONS = 0;
constexpr int TEXOORDS  = 1;
constexpr int NORMALS   = 2;
constexpr int TANGENTS  = 3;

// Texture Units
constexpr int DIFFUSE_TEX_UNIT    = 0;
constexpr int SPECULAR_TEX_UNIT   = 1;
constexpr int GLOSSY_TEX_UNIT     = 2;
constexpr int NORMAL_TEX_UNIT     = 3;
constexpr int EMISSIVE_TEX_UNIT   = 4;
constexpr int ALPHA_TEX_UNIT      = 5;
constexpr int AMBIENT_TEX_UNIT    = 6;
constexpr int NUM_TEXT_UNITS      = 7; // <- keep up to date

constexpr int DIR_LIGHT_TEX_UNIT  = 7;
constexpr int OMNI_LIGHT_TEX_UNIT = 8;

} // namespace bindings

namespace shader
{

struct CameraStruct
{
    static constexpr int alignment() {return 64;} // ??? not sure why, but glBindBufferRange
                                                  // doesn't accept other alignments
    glm::mat4                       ViewMatrix;
    glm::mat4                       ProjMatrix;
    glm::mat4                       ProjViewMatrix;
    glm::vec4                       CameraPosition;
    glm::vec4                       padding[3];
};
static_assert(sizeof(CameraStruct) == 256 &&
        sizeof(CameraStruct) % CameraStruct::alignment() == 0, "");


struct MaterialStruct
{
    static constexpr int alignment() {return 16;}
    GLfloat                         diffuseColor[3];
    GLint                           hasDiffuseTex;

    GLfloat                         specularColor[3];
    GLint                           hasSpecularTex;

    GLfloat                         ambientColor[3];
    GLint                           hasAmbientTex;

    GLfloat                         emissiveColor[3];
    GLint                           hasEmissiveTex;

    GLfloat                         transparentColor[3];
    GLint                           hasAlphaTex;


    GLint                           hasGlossyTex;
    GLint                           hasNormalTex;
    GLfloat                         glossiness;
    GLfloat                         opacity;
};
static_assert(sizeof(MaterialStruct) == 96 &&
        sizeof(MaterialStruct) % MaterialStruct::alignment() == 0, "");

struct InstanceStruct
{
    static constexpr int alignment() {return  16;}
    glm::mat4                       ModelMatrix;
    GLfloat                         BBox_min[3];
    GLuint                          MeshID;
    GLfloat                         BBox_max[3];
    GLuint                          MaterialID;
};
static_assert(sizeof(InstanceStruct) == 96 &&
        sizeof(InstanceStruct) % InstanceStruct::alignment() == 0, "");

struct MeshStruct
{
    static constexpr int alignment() {return 4;}
    GLuint                          stride;
    GLuint                          components; // see mesh.h
    GLuint                          first;
};
static_assert(sizeof(MeshStruct) == 12 &&
        sizeof(MeshStruct) % MeshStruct::alignment() == 0, "");

struct LightStruct
{
    static constexpr int alignment() {return 16;} // vec4
    enum Type : GLint {SPOT        = 0x10000000,
                       DIRECTIONAL = 0x20000000,
                       POINT       = 0x40000000};

    glm::mat4                       projViewMatrix;

    glm::vec3                       position;
    GLfloat                         angleInnerCone;

    glm::vec3                       direction;
    GLfloat                         angleOuterCone;

    glm::vec3                       intensity;
    GLfloat                         maxDistance;

    GLfloat                         constantAttenuation;
    GLfloat                         linearAttenuation;
    GLfloat                         quadraticAttenuation;
    GLint                           type_texid; // 31:      is shadowcasting
                                                // [30:28]: type
                                                // [27:0]:  depth texture index
};
static_assert(sizeof(LightStruct) == 128 &&
        sizeof(LightStruct) % LightStruct::alignment() == 0, "");

} // namespace shader

} // namespace core

#endif // CORE_OPENGL_INTERFACE_H

