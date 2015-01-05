#include <utility>
#include <cassert>
#include <cstring>
#include "texture.h"
#include "texture_manager.h"

namespace core
{

/****************************************************************************/

Texture::Texture()
  : m_texture{gl::NO_GEN},
    m_handle{0},
    m_num_channels{0},
    m_index{0}
{
}

/****************************************************************************/

Texture::Texture(gl::Texture&& texture, const GLuint64 handle,
        const GLuint index, const GLuint num_channels)
  : m_texture{std::move(texture)},
    m_handle{handle},
    m_num_channels{num_channels},
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

GLuint Texture::getNumChannels() const
{
    return m_num_channels;
}

/****************************************************************************/

GLuint Texture::getTextureIndex() const
{
    return m_index;
}

/****************************************************************************/

} // namespace core
