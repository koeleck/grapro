#include <utility>
#include <cassert>
#include <cstring>
#include "texture.h"
#include "shader_interface.h"
#include "texture_manager.h"

namespace core
{

/****************************************************************************/

Texture::Texture(gl::Texture&& texture)
  : m_texture{std::move(texture)},
    m_handle{0},
    m_index{-1}
{
}

/****************************************************************************/

Texture::~Texture()
{
    if (m_index != -1) {
        const auto offset = m_index * static_cast<GLintptr>(sizeof(shader::TextureStruct));
        res::textures->m_texture_buffer.free(offset);
    }
}

/****************************************************************************/

Texture::operator GLuint() const
{
    return m_texture.get();
}

/****************************************************************************/

GLuint64 Texture::getTextureHandle() const
{
    makeResident();
    return m_handle;
}

/****************************************************************************/

GLintptr Texture::getTextureIndex() const
{
    makeResident();
    return m_index;
}

/****************************************************************************/

void Texture::makeResident() const
{
    constexpr int size = sizeof(shader::TextureStruct);
    if (m_index == -1)
        return;

    m_handle = glGetTextureHandleARB(m_texture);
    glMakeTextureHandleResidentARB(m_handle);

    GLintptr offset =
        res::textures->m_texture_buffer.alloc(size, size);

    shader::TextureStruct tex_info;
    tex_info.handle = *reinterpret_cast<glm::uvec2*>(&m_handle);
    tex_info.num_channels = 0; // TODO

    std::memcpy(res::textures->m_texture_buffer.offsetToPointer(offset),
            &tex_info, sizeof(shader::TextureStruct));

}

/****************************************************************************/

} // namespace core
