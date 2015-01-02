#include <utility>
#include <cassert>
#include <cstring>
#include "texture.h"
#include "shader_interface.h"
#include "texture_manager.h"

namespace core
{

/****************************************************************************/

Texture::Texture(gl::Texture&& texture, const GLuint64 handle, const GLuint index)
  : m_texture{std::move(texture)},
    m_handle{handle},
    m_index{index}
{
}

/****************************************************************************/

Texture::~Texture()
{
}

/****************************************************************************/

Texture::operator GLuint() const
{
    return m_texture.get();
}

/****************************************************************************/

GLuint64 Texture::getTextureHandle() const
{
    return m_handle;
}

/****************************************************************************/

GLuint Texture::getTextureIndex() const
{
    return m_index;
}

/****************************************************************************/

} // namespace core
