#ifndef CORE_MATERIAL_MANAGER_H
#define CORE_MATERIAL_MANAGER_H

#include <unordered_map>
#include <memory>
#include <string>
#include "managers.h"
#include "shader_interface.h"
#include "buffer_storage_pool.h"
#include "material.h"

namespace import
{
struct Material;
struct Texture;
} // namespace import

namespace core
{

class MaterialManager
{
public:
    MaterialManager();
    ~MaterialManager();

    Material* addMaterial(const std::string& name, const import::Material* material,
            const import::Texture* const * textures);
    Material* addMaterial(const std::string& name);

    Material* getMaterial(const std::string& name);
    const Material* getMaterial(const std::string& name) const;

    void bind() const;

private:
    using MaterialPool = BufferStoragePool<shader::MaterialStruct>;
    using MaterialMap = std::unordered_map<std::string, std::unique_ptr<Material>>;

    MaterialPool        m_material_buffer;
    MaterialMap         m_materials;
};

} // namespace core

#endif // CORE_MATERIAL_MANAGER_H

