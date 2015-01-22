#include "material_manager.h"
#include "texture_manager.h"

#include "log/log.h"
#include "import/material.h"
#include "import/texture.h"

namespace core
{

/****************************************************************************/

constexpr int MAX_NUM_MATERIALS = 256;
MaterialManager::MaterialManager()
  : m_material_buffer(GL_SHADER_STORAGE_BUFFER, MAX_NUM_MATERIALS)
{
    bind();
}

/****************************************************************************/

MaterialManager::~MaterialManager() = default;

/****************************************************************************/

Material* MaterialManager::addMaterial(const std::string& name,
        const import::Material* material,
        const import::Texture* const* textures)
{
    Material* result;
    auto it = m_materials.find(name);
    if (it != m_materials.end()) {
        LOG_WARNING("Overwriting existing material '", name,
                "' with imported material: ", material->name);
        result = it->second.get();
    } else {
        const GLintptr offset = m_material_buffer.alloc();
        const auto index = m_material_buffer.offsetToIndex(offset);
        auto* ptr = m_material_buffer.offsetToPointer(offset);

        std::unique_ptr<Material> mat{new Material(index, ptr)};
        auto res = m_materials.emplace(name, std::move(mat));
        result = res.first->second.get();
    }

    if (material->hasDiffuseTexture()) {
        result->setDiffuseTexture(res::textures->getTexture(textures[material->diffuse_texture]->name));
    }
    if (material->hasSpecularTexture()) {
        result->setSpecularTexture(res::textures->getTexture(textures[material->specular_texture]->name));
    }
    if (material->hasExponentTexture()) {
        result->setGlossyTexture(res::textures->getTexture(textures[material->emissive_texture]->name));
    }
    if (material->hasNormalTexture()) {
        result->setNormalTexture(res::textures->getTexture(textures[material->normal_texture]->name));
    }
    if (material->hasAlphaTexture()) {
        result->setAlphaTexture(res::textures->getTexture(textures[material->alpha_texture]->name));
    }
    if (material->hasEmissiveTexture()) {
        result->setEmissiveTexture(res::textures->getTexture(textures[material->emissive_texture]->name));
    }
    if (material->hasAmbientTexture()) {
        result->setAmbientTexture(res::textures->getTexture(textures[material->ambient_texture]->name));
    }
    result->setDiffuseColor(material->diffuse_color);
    result->setSpecularColor(material->specular_color);
    result->setAmbientColor(material->ambient_color);
    result->setEmissiveColor(material->emissive_color);
    result->setTransparentColor(material->transparent_color);
    result->setGlossiness(material->specular_exponent);
    result->setOpacity(material->opacity);

    return result;
}

/****************************************************************************/

Material* MaterialManager::addMaterial(const std::string& name)
{
    auto it = m_materials.find(name);
    if (it != m_materials.end()) {
        LOG_INFO("Material already exists: ", name);
        return it->second.get();
    }

    const GLintptr offset = m_material_buffer.alloc();
    const auto index = m_material_buffer.offsetToIndex(offset);
    auto* ptr = m_material_buffer.offsetToPointer(offset);

    std::unique_ptr<Material> mat{new Material(index, ptr)};
    auto res = m_materials.emplace(name, std::move(mat));
    return res.first->second.get();
}

/****************************************************************************/

Material* MaterialManager::getMaterial(const std::string& name)
{
    auto it = m_materials.find(name);
    if (it == m_materials.end()) {
        LOG_ERROR("Material not found: ", name);
        return nullptr;
    }
    return it->second.get();
}

/****************************************************************************/

const Material* MaterialManager::getMaterial(const std::string& name) const
{
    auto it = m_materials.find(name);
    if (it == m_materials.end()) {
        LOG_ERROR("Material not found: ", name);
        return nullptr;
    }
    return it->second.get();
}

/****************************************************************************/

void MaterialManager::bind() const
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindings::MATERIAL,
            m_material_buffer.buffer());
}

/****************************************************************************/

} // namespace core

