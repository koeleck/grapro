#include <cstring>

#include "texture_manager.h"
#include "shader_interface.h"
#include "log/log.h"
#include "import/image.h"
#include "framework/vars.h"

namespace core
{

/****************************************************************************/

static constexpr int MAX_NUM_TEXTURES = 1024;

TextureManager::TextureManager()
  : m_texture_buffer(GL_SHADER_STORAGE_BUFFER, MAX_NUM_TEXTURES)
{
    // reserve the first texture as sort of nullptr
    auto offset = m_texture_buffer.alloc();
    assert(offset == 0);
}

/****************************************************************************/

TextureManager::~TextureManager() = default;

/****************************************************************************/

void TextureManager::addTexture(const std::string& name, const import::Image& image)
{
    const int numChannels = image.numChannels();
    GLenum internal_format = GL_RGBA8;
    if (numChannels == 1)
        internal_format = GL_R8;
    else if (numChannels == 3)
        internal_format = GL_RGB8;
    gl::Texture tex;
    glTextureStorage2DEXT(tex, GL_TEXTURE_2D, image.maxNumMipMaps(),
            internal_format, image.width(), image.height());

    glTextureParameteriEXT(tex, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
            gl::stringToEnum(vars.tex_min_filter));
    glTextureParameteriEXT(tex, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
            gl::stringToEnum(vars.tex_mag_filter));
    glTextureParameterfEXT(tex, GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
            vars.tex_max_anisotropy);
    glTextureParameteriEXT(tex, GL_TEXTURE_2D,  GL_TEXTURE_WRAP_S,
             GL_REPEAT);
    glTextureParameteriEXT(tex, GL_TEXTURE_2D,  GL_TEXTURE_WRAP_T,
             GL_REPEAT);

    glTextureSubImage2DEXT(tex, GL_TEXTURE_2D, 0, 0, 0, image.width(), image.height(),
            image.gl_format(), image.gl_type(), image.bits());
    glGenerateTextureMipmapEXT(tex, GL_TEXTURE_2D);


    addTexture(name, std::move(tex), numChannels);
}

/****************************************************************************/

void TextureManager::addTexture(const std::string& name, gl::Texture&& texture,
        const int num_channels)
{
    auto it = m_textures.find(name);
    if (it != m_textures.end()) {
        LOG_ERROR("Texture already exists: ", name);
        abort();
    }

    GLuint64 handle = glGetTextureHandleARB(texture);
    glMakeTextureHandleResidentARB(handle);
    GLintptr offset = m_texture_buffer.alloc();
    GLintptr index = offset / static_cast<GLintptr>(sizeof(shader::TextureStruct));

    shader::TextureStruct dta;
    dta.handle = *reinterpret_cast<glm::uvec2*>(&handle);
    dta.num_channels = num_channels;

    std::memcpy(m_texture_buffer.offsetToPointer(offset), &dta, sizeof(shader::TextureStruct));

    m_textures.emplace(name, std::unique_ptr<Texture>(new Texture(std::move(texture), handle, index)));
}

/****************************************************************************/

Texture* TextureManager::getTexture(const std::string& name)
{
    auto it = m_textures.find(name);
    if (it == m_textures.end())
        return nullptr;
    return it->second.get();
}

/****************************************************************************/

const Texture* TextureManager::getTexture(const std::string& name) const
{
    auto it = m_textures.find(name);
    if (it == m_textures.end())
        return nullptr;
    return it->second.get();
}

/****************************************************************************/

} // namespace core
