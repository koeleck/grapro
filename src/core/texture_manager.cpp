#include "texture_manager.h"
#include "log/log.h"
#include "import/image.h"
#include "framework/vars.h"

namespace core
{

/****************************************************************************/

static constexpr int MAX_NUM_TEXTURES = 1024;

TextureManager::TextureManager()
{
}

/****************************************************************************/

TextureManager::~TextureManager() = default;

/****************************************************************************/

Texture* TextureManager::addTexture(const std::string& name, const import::Image& image)
{
    const int numChannels = image.numChannels();
    GLenum internal_format = GL_RGBA8;
    if (numChannels == 1)
        internal_format = GL_R8;
    else if (numChannels == 3)
        internal_format = GL_RGB8;

    gl::Texture tex;
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexStorage2D(GL_TEXTURE_2D, image.maxNumMipMaps(),
            internal_format, image.width(), image.height());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
            static_cast<GLint>(gl::stringToEnum(vars.tex_min_filter)));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
            static_cast<GLint>(gl::stringToEnum(vars.tex_mag_filter)));
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
            vars.tex_max_anisotropy);
    glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_WRAP_S,
             GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_WRAP_T,
             GL_REPEAT);

    glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0,
            static_cast<GLsizei>(image.width()), static_cast<GLsizei>(image.height()),
            image.gl_format(), image.gl_type(), image.bits());
    glGenerateMipmap(GL_TEXTURE_2D);

    //glTextureStorage2DEXT(tex, GL_TEXTURE_2D, image.maxNumMipMaps(),
    //        internal_format, image.width(), image.height());

    //glTextureParameteriEXT(tex, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    //        gl::stringToEnum(vars.tex_min_filter));
    //glTextureParameteriEXT(tex, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
    //        gl::stringToEnum(vars.tex_mag_filter));
    //glTextureParameterfEXT(tex, GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
    //        vars.tex_max_anisotropy);
    //glTextureParameteriEXT(tex, GL_TEXTURE_2D,  GL_TEXTURE_WRAP_S,
    //         GL_REPEAT);
    //glTextureParameteriEXT(tex, GL_TEXTURE_2D,  GL_TEXTURE_WRAP_T,
    //         GL_REPEAT);

    //glTextureSubImage2DEXT(tex, GL_TEXTURE_2D, 0, 0, 0, image.width(), image.height(),
    //        image.gl_format(), image.gl_type(), image.bits());
    //glGenerateTextureMipmapEXT(tex, GL_TEXTURE_2D);


    return addTexture(name, std::move(tex), numChannels);
}

/****************************************************************************/

Texture* TextureManager::addTexture(const std::string& name, gl::Texture&& texture,
        const int num_channels)
{
    auto it = m_textures.find(name);
    if (it != m_textures.end()) {
        LOG_WARNING("Overwriting already existing texture: ", name);
    }

    auto res = m_textures.emplace(name, std::unique_ptr<Texture>(new Texture(std::move(texture),
                    static_cast<GLuint>(num_channels))));
    return res.first->second.get();
}

/****************************************************************************/

Texture* TextureManager::getTexture(const std::string& name)
{
    auto it = m_textures.find(name);
    if (it == m_textures.end()) {
        LOG_ERROR("Unkown texture: ", name);
        return nullptr;
    }
    return it->second.get();
}

/****************************************************************************/

const Texture* TextureManager::getTexture(const std::string& name) const
{
    auto it = m_textures.find(name);
    if (it == m_textures.end()) {
        LOG_ERROR("Unkown texture: ", name);
        return nullptr;
    }
    return it->second.get();
}

/****************************************************************************/

} // namespace core
