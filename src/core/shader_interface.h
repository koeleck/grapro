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

struct MeshStruct
{
    GLuint                          first;
    GLuint                          stride;

    // (0) normal (1) texcoord (2) tangent (3) vertex color
    GLboolean                       components[4];
};

struct TextureStruct
{
    GLuint64                        handle;
    GLuint                          num_channels;
    GLuint                          padding;
};

} // namespace shader

} // namespace core

#endif // CORE_OPENGL_INTERFACE_H