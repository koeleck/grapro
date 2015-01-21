#include <cstring>

#include "material.h"
#include "shader_interface.h"
#include "texture.h"

namespace core
{

/****************************************************************************/

Material::Material(const GLuint index, shader::MaterialStruct* const ptr)
  : m_diffuse_texture{nullptr},
    m_specular_texture{nullptr},
    m_glossy_texture{nullptr},
    m_normal_texture{nullptr},
    m_emissive_texture{nullptr},
    m_alpha_texture{nullptr},
    m_ambient_texture{nullptr},
    m_diffuse_color{1.f},
    m_specular_color{1.f},
    m_ambient_color{.0f},
    m_emissive_color{.0f},
    m_transparent_color{.0f},
    m_glossiness{1.f},
    m_opacity{1.f},
    m_index{index},
    m_data{ptr}
{
    shader::MaterialStruct data;
    data.hasDiffuseTex = 0;
    data.hasSpecularTex = 0;
    data.hasGlossyTex = 0;
    data.hasNormalTex = 0;
    data.hasEmissiveTex = 0;
    data.hasAlphaTex = 0;
    data.hasAmbientTex = 0;
    data.diffuseColor[0] = m_diffuse_color.x;
    data.diffuseColor[1] = m_diffuse_color.y;
    data.diffuseColor[2] = m_diffuse_color.z;
    data.specularColor[0] = m_specular_color.x;
    data.specularColor[1] = m_specular_color.y;
    data.specularColor[2] = m_specular_color.z;
    data.ambientColor[0] = m_ambient_color.x;
    data.ambientColor[1] = m_ambient_color.y;
    data.ambientColor[2] = m_ambient_color.z;
    data.emissiveColor[0] = m_emissive_color.x;
    data.emissiveColor[1] = m_emissive_color.y;
    data.emissiveColor[2] = m_emissive_color.z;
    data.transparentColor[0] = m_transparent_color.x;
    data.transparentColor[1] = m_transparent_color.y;
    data.transparentColor[2] = m_transparent_color.z;
    data.glossiness = m_glossiness;
    data.opacity = m_opacity;

    std::memcpy(m_data, &data, sizeof(shader::MaterialStruct));
}

/****************************************************************************/

bool Material::hasDiffuseTexture() const
{
    return m_diffuse_texture != nullptr;
}

/****************************************************************************/

bool Material::hasSpecularTexture() const
{
    return m_specular_texture != nullptr;
}

/****************************************************************************/

bool Material::hasEmissiveTexture() const
{
    return m_emissive_texture != nullptr;
}

/****************************************************************************/

bool Material::hasGlossyTexture() const
{
    return m_glossy_texture != nullptr;
}

/****************************************************************************/

bool Material::hasAlphaTexture() const
{
    return m_alpha_texture != nullptr;
}

/****************************************************************************/

bool Material::hasNormalTexture() const
{
    return m_normal_texture != nullptr;
}

/****************************************************************************/

bool Material::hasAmbientTexture() const
{
    return m_ambient_texture != nullptr;
}

/****************************************************************************/


const Texture* Material::getDiffuseTexture() const
{
    return m_diffuse_texture;
}

/****************************************************************************/

const Texture* Material::getSpecularTexture() const
{
    return m_diffuse_texture;
}

/****************************************************************************/

const Texture* Material::getEmissiveTexture() const
{
    return m_emissive_texture;
}

/****************************************************************************/

const Texture* Material::getGlossyTexture() const
{
    return m_glossy_texture;
}

/****************************************************************************/

const Texture* Material::getAlphaTexture() const
{
    return m_alpha_texture;
}

/****************************************************************************/

const Texture* Material::getNormalTexture() const
{
    return m_normal_texture;
}

/****************************************************************************/

const Texture* Material::getAmbientTexture() const
{
    return m_ambient_texture;
}

/****************************************************************************/

void Material::setDiffuseTexture(const Texture* texture)
{
    m_diffuse_texture = texture;
    m_data->hasDiffuseTex = (texture == nullptr) ? 0 : -1;
}

/****************************************************************************/

void Material::setSpecularTexture(const Texture* texture)
{
    m_specular_texture = texture;
    m_data->hasSpecularTex = (texture == nullptr) ? 0 : -1;
}

/****************************************************************************/

void Material::setEmissiveTexture(const Texture* texture)
{
    m_emissive_texture = texture;
    m_data->hasEmissiveTex = (texture == nullptr) ? 0 : -1;
}

/****************************************************************************/

void Material::setGlossyTexture(const Texture* texture)
{
    m_glossy_texture = texture;
    m_data->hasGlossyTex = (texture == nullptr) ? 0 : -1;
}

/****************************************************************************/

void Material::setAlphaTexture(const Texture* texture)
{
    m_alpha_texture = texture;
    m_data->hasAlphaTex = (texture == nullptr) ? 0 : -1;
}

/****************************************************************************/

void Material::setNormalTexture(const Texture* texture)
{
    m_normal_texture = texture;
    m_data->hasNormalTex = (texture == nullptr) ? 0 : -1;
}

/****************************************************************************/

void Material::setAmbientTexture(const Texture* texture)
{
    m_ambient_texture = texture;
    m_data->hasAmbientTex = (texture == nullptr) ? 0 : -1;
}

/****************************************************************************/

const glm::vec3& Material::getDiffuseColor() const
{
    return m_diffuse_color;
}

/****************************************************************************/

const glm::vec3& Material::getSpecularColor() const
{
    return m_specular_color;
}

/****************************************************************************/

const glm::vec3& Material::getAmbientColor() const
{
    return m_ambient_color;
}

/****************************************************************************/

const glm::vec3& Material::getEmissiveColor() const
{
    return m_emissive_color;
}

/****************************************************************************/

const glm::vec3& Material::getTransparentColor() const
{
    return m_transparent_color;
}

/****************************************************************************/

float Material::getGlossiness() const
{
    return m_glossiness;
}

/****************************************************************************/

float Material::getOpacity() const
{
    return m_opacity;
}

/****************************************************************************/

void Material::setDiffuseColor(const glm::vec3& color)
{
    m_diffuse_color = color;
    m_data->diffuseColor[0] = color.x;
    m_data->diffuseColor[1] = color.y;
    m_data->diffuseColor[2] = color.z;
}

/****************************************************************************/

void Material::setSpecularColor(const glm::vec3& color)
{
    m_specular_color = color;
    m_data->specularColor[0] = color.x;
    m_data->specularColor[1] = color.y;
    m_data->specularColor[2] = color.z;
}

/****************************************************************************/

void Material::setAmbientColor(const glm::vec3& color)
{
    m_ambient_color = color;
    m_data->ambientColor[0] = color.x;
    m_data->ambientColor[1] = color.y;
    m_data->ambientColor[2] = color.z;
}

/****************************************************************************/

void Material::setEmissiveColor(const glm::vec3& color)
{
    m_emissive_color = color;
    m_data->emissiveColor[0] = color.x;
    m_data->emissiveColor[1] = color.y;
    m_data->emissiveColor[2] = color.z;
}

/****************************************************************************/

void Material::setTransparentColor(const glm::vec3& color)
{
    m_transparent_color = color;
    m_data->transparentColor[0] = color.x;
    m_data->transparentColor[1] = color.y;
    m_data->transparentColor[2] = color.z;
}

/****************************************************************************/

void Material::setGlossiness(const float value)
{
    m_glossiness = value;
    m_data->glossiness = value;
}

/****************************************************************************/

void Material::setOpacity(const float value)
{
    m_opacity = value;
    m_data->opacity = value;
}

/****************************************************************************/

GLuint Material::getIndex() const
{
    return m_index;
}

/****************************************************************************/

bool Material::isCompatible(const Material& other) const
{
    if (this == &other)
        return true;

    // compatible, if using the same textures, or one uses textures and the
    // other doesn't, or both don't use any textures at all
    if (hasDiffuseTexture() && other.hasDiffuseTexture()) {
        if (getDiffuseTexture() != other.getDiffuseTexture())
            return false;
    }
    if (hasSpecularTexture() && other.hasSpecularTexture()) {
        if (getSpecularTexture() != other.getSpecularTexture())
            return false;
    }
    if (hasEmissiveTexture() && other.hasEmissiveTexture()) {
        if (getEmissiveTexture() != other.getEmissiveTexture())
            return false;

    }
    if (hasGlossyTexture() && other.hasGlossyTexture()) {
        if (getGlossyTexture() != other.getGlossyTexture())
            return false;
    }
    if (hasAlphaTexture() && other.hasAlphaTexture()) {
        if (getAlphaTexture() != other.getAlphaTexture())
            return false;
    }
    if (hasNormalTexture() && other.hasNormalTexture()) {
        if (getNormalTexture() != other.getNormalTexture())
            return false;
    }
    if (hasAmbientTexture() && other.hasAmbientTexture()) {
        if (getAmbientTexture() != other.getAmbientTexture())
            return false;
    }

    return true;
}

/****************************************************************************/

} // namespace core

