#ifndef CORE_TEXTURE_MANAGER_H
#define CORE_TEXTURE_MANAGER_H

#include <unordered_map>
#include <string>
#include <memory>

#include "managers.h"
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

    Texture* addTexture(const std::string& name, const import::Image& image);
    Texture* addTexture(const std::string& name, gl::Texture&& texture, int num_channels);

    Texture* getTexture(const std::string& name);
    const Texture* getTexture(const std::string& name) const;

private:
    friend class Texture;

    using TextureMap = std::unordered_map<std::string, std::unique_ptr<Texture>>;

    TextureMap                              m_textures;
};

} // namespace core

#endif // CORE_TEXTURE_MANAGER_H

