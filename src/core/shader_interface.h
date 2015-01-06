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

// Vertex Attrib Arrays
constexpr int POSITIONS = 0;
constexpr int NORMALS   = 1;
constexpr int TEXOORDS  = 2;
constexpr int TANGENTS  = 3;

// Texture Units
constexpr int DIFFUSE_TEX_UNIT  = 0;
constexpr int SPECULAR_TEX_UNIT = 1;
constexpr int GLOSSY_TEX_UNIT   = 2;
constexpr int NORMAL_TEX_UNIT   = 3;
constexpr int EMISSIVE_TEX_UNIT = 4;
constexpr int ALPHA_TEX_UNIT    = 5;
constexpr int AMBIENT_TEX_UNIT  = 6;
constexpr int NUM_TEXT_UNITS    = 7; // <- keep up to date

} // namespace bindings

namespace shader
{

struct CameraStruct
{
    glm::mat4                       ViewMatrix;
    glm::mat4                       ProjMatrix;
    glm::mat4                       ProjViewMatrix;
    glm::vec4                       CameraPosition;
};
static_assert(sizeof(CameraStruct) == 208 && sizeof(CameraStruct) % 16 == 0, "");


struct MaterialStruct
{
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
static_assert(sizeof(MaterialStruct) == 96 && sizeof(MaterialStruct) % 4 == 0, "");


} // namespace shader

} // namespace core

#endif // CORE_OPENGL_INTERFACE_H

