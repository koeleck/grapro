#ifndef CORE_OPENGL_INTERFACE_H
#define CORE_OPENGL_INTERFACE_H

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
constexpr int CAMERA = 0;
constexpr int TEXTURE = 1;

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
};

struct TextureStruct
{
    glm::uvec2                      handle;
    glm::int32_t                    num_channels;
    glm::int32_t                    padding;
};

} // namespace shader

} // namespace core

#endif // CORE_OPENGL_INTERFACE_H
