#ifndef CORE_MESH_MANAGER_H
#define CORE_MESH_MANAGER_H

#include <string>
#include <unordered_map>
#include <memory>

#include "gl/gl_objects.h"
#include "mesh.h"
#include "managers.h"
#include "buffer_storage.h"
#include "buffer_storage_pool.h"
#include "shader_interface.h"

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

    GLuint getVAO(const Mesh* mesh) const;
    GLuint getElementArrayBuffer() const;

    void bind() const;

private:
    using MeshPool = BufferStoragePool<sizeof(shader::MeshStruct)>;
    using MeshMap = std::unordered_map<std::string, std::unique_ptr<Mesh>>;
    using VAOMap = std::unordered_map<unsigned char, gl::VertexArray>;

    void initVAOs();

    MeshMap             m_meshes;
    BufferStorage       m_data;
    VAOMap              m_vaos;
    MeshPool            m_mesh_pool;
};

} // namespace core

#endif // CORE_MESH_MANAGER_H

