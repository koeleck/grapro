#ifndef CORE_TEXTURE_MANAGER_H
#define CORE_TEXTURE_MANAGER_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "managers.h"
#include "buffer_storage_pool.h"
#include "gl/gl_objects.h"
#include "texture.h"
#include "shader_interface.h"

namespace import
{
class Image;
}

namespace core
{

class TextureManager
{
public:
    TextureManager();
    ~TextureManager();

    Texture* addTexture(const std::string& name, const import::Image& image);
    Texture* addTexture(const std::string& name, gl::Texture&& texture, int num_channels);

    Texture* getTexture(const std::string& name);
    const Texture* getTexture(const std::string& name) const;

    void bind() const;

private:
    friend class Texture;

    using TextureMap = std::unordered_map<std::string, std::unique_ptr<Texture>>;
    using TexturePool = BufferStoragePool<sizeof(shader::TextureStruct)>;

    TextureMap                              m_textures;
    TexturePool                             m_texture_buffer;
};

} // namespace core

#endif // CORE_TEXTURE_MANAGER_H

