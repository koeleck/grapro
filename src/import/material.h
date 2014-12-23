#ifndef IMPORT_MATERIAL_H
#define IMPORT_MATERIAL_H

#include <cstdint>
#include <glm/glm.hpp>

namespace import
{

struct Material
{
    static constexpr int VERSION = 1;

    const char*     name;
    std::int32_t    diffuse_texture;
    std::int32_t    specular_texture;
    std::int32_t    exponent_texture;
    std::int32_t    normal_texture;
    std::int32_t    emissive_texture;
    std::int32_t    alpha_texture;
    glm::vec3       diffuse_color;
    glm::vec3       specular_color;
    glm::vec3       ambient_color;
    glm::vec3       emissive_color;
    glm::vec3       transparent_color;
    float           specular_exponent;
    float           opacity;

    bool hasDiffuseTexture() const noexcept
    {
        return diffuse_texture != -1;
    }

    bool hasSpecularTexture() const noexcept
    {
        return specular_texture != -1;
    }

    bool hasExponentTexture() const noexcept
    {
        return exponent_texture != -1;
    }

    bool hasNormalTexture() const noexcept
    {
        return normal_texture != -1;
    }

    bool hasEmissiveTexture() const noexcept
    {
        return emissive_texture != -1;
    }

    bool hasAlphaTexture() const noexcept
    {
        return alpha_texture != -1;
    }

    bool isTransparent() const noexcept
    {
        return opacity < 1.f;
    }

    bool isEmissive() const noexcept
    {
        return hasEmissiveTexture() ||
            glm::any(glm::greaterThan(emissive_color, glm::vec3(.0f)));
    }
};

} // namespace import

#endif // IMPORT_MATERIAL_H

