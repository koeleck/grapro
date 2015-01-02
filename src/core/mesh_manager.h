#ifndef CORE_MESH_MANAGER_H
#define CORE_MESH_MANAGER_H

#include <string>
#include <unordered_map>
#include <memory>
#include "buffer_storage.h"
#include "buffer_storage_pool.h"
#include "shader_interface.h"
#include "mesh.h"
#include "managers.h"

namespace import
{
class Mesh;
} // namespace import

namespace core
{

class MeshManager
{
public:
    MeshManager();
    ~MeshManager();

    Mesh* addMesh(const import::Mesh* mesh);
    Mesh* getMesh(const char* name);
    const Mesh* getMesh(const char* name) const;

private:
    using MeshMap = std::unordered_map<std::string, std::unique_ptr<Mesh>>;
    using MeshPool = BufferStoragePool<sizeof(shader::MeshStruct)>;

    MeshMap             m_meshes;
    BufferStorage       m_vertex_buffer;
    MeshPool            m_mesh_buffer;
};

} // namespace core

#endif // CORE_MESH_MANAGER_H

