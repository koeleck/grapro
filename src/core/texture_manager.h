#ifndef CORE_TEXTURE_MANAGER_H
#define CORE_TEXTURE_MANAGER_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "managers.h"
#include "buffer_storage.h"
#include "gl/gl_objects.h"
#include "texture.h"

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

    void addTexture(const std::string& name, const import::Image& image);
    void addTexture(const std::string& name, gl::Texture&& texture);

    Texture* getTexture(const std::string& name);
    const Texture* getTexture(const std::string& name) const;

private:
    friend class Texture;

    using TextureMap = std::unordered_map<std::string, std::unique_ptr<Texture>>;

    TextureMap                              m_textures;
    BufferStorage                           m_texture_buffer;
};

} // namespace core

#endif // CORE_TEXTURE_MANAGER_H

