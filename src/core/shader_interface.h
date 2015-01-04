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

struct MeshStruct
{
    static_assert(4 * sizeof(GLboolean) == sizeof(GLuint), "");
    GLuint                          first;
    GLuint                          stride;

    // (0) normal (1) texcoord (2) tangent (3) vertex color
    GLboolean                       components[4];
};
static_assert(sizeof(MeshStruct) == 12 && sizeof(MeshStruct) % 4 == 0, "");

struct TextureStruct
{
    GLuint64                        handle;
    GLuint                          num_channels;
    GLuint                          padding;
};
static_assert(sizeof(TextureStruct) == 16 && sizeof(TextureStruct) % 8 == 0, "");

struct MaterialStruct
{
    GLuint                          diffuseTex;
    GLuint                          specularTex;
    GLuint                          glossyTex;
    GLuint                          normalTex;
    GLuint                          emissiveTex;
    GLuint                          alphaTex;
    GLfloat                         diffuseColor[4];
    GLfloat                         specularColor[4];
    GLfloat                         ambientColor[4];
    GLfloat                         emissiveColor[4];
    GLfloat                         transparentColor[4];
    GLfloat                         glossiness;
    GLfloat                         opacity;
};
static_assert(sizeof(MaterialStruct) == 112 && sizeof(MaterialStruct) % 4 == 0, "");

struct InstanceStruct
{
    glm::mat4                       modelMatrix;
    GLuint                          meshID;
    GLuint                          materialID;
    GLuint                          _padding[2];
};
static_assert(sizeof(InstanceStruct) == 80 && sizeof(InstanceStruct) % 16 == 0, "");

} // namespace shader

} // namespace core

#endif // CORE_OPENGL_INTERFACE_H
