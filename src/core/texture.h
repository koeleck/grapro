#ifndef CORE_TEXTURE_H
#define CORE_TEXTURE_H

#include "gl/gl_objects.h"

namespace core
{

class Texture
{
public:
    ~Texture();

    operator GLuint() const;

    GLuint64 getTextureHandle() const;
    GLintptr getTextureIndex() const;

private:
    friend class TextureManager;
    Texture(gl::Texture&&, GLuint64, GLintptr);

    gl::Texture m_texture;
    GLuint64    m_handle;
    GLintptr    m_index;
};

} // namespace core

#endif // CORE_TEXTURE_H

