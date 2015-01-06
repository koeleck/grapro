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
    m_num_channels{0}
{
}

/****************************************************************************/

Texture::Texture(gl::Texture&& texture, const GLuint num_channels)
  : m_texture{std::move(texture)},
    m_num_channels{num_channels}
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

GLuint Texture::getNumChannels() const
{
    return m_num_channels;
}

/****************************************************************************/

} // namespace core
