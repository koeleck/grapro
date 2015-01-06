#ifndef CORE_TEXTURE_H
#define CORE_TEXTURE_H

#include "gl/gl_objects.h"

namespace core
{

class Texture
{
public:
    Texture();
    ~Texture();

    operator GLuint() const;
    GLuint getNumChannels() const;

private:
    friend class TextureManager;
    Texture(gl::Texture&&, GLuint num_channels);

    gl::Texture m_texture;
    GLuint      m_num_channels;
};

} // namespace core

#endif // CORE_TEXTURE_H

