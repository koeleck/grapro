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
    Texture(gl::Texture&&);

    void makeResident() const;

    gl::Texture         m_texture;
    mutable GLuint64    m_handle;
    mutable GLintptr    m_index;
};

} // namespace core

#endif // CORE_TEXTURE_H

