#include "mesh_manager.h"
#include "import/mesh.h"
#include "log/log.h"
#include "framework/vars.h"

namespace core
{

/****************************************************************************/

constexpr int MAX_NUM_MESHES = 1024;
MeshManager::MeshManager()
  : m_vertex_buffer(GL_SHADER_STORAGE_BUFFER, static_cast<std::size_t>(vars.vertex_buffer_size)),
    m_mesh_buffer(GL_SHADER_STORAGE_BUFFER, MAX_NUM_MESHES)
{
}

/****************************************************************************/

MeshManager::~MeshManager() = default;

/****************************************************************************/

Mesh* MeshManager::addMesh(const import::Mesh* mesh)
{
    std::string name = mesh->name;

    auto it = m_meshes.find(name);
    if (it != m_meshes.end()) {
        LOG_INFO("Mesh already added: ", name);
        return it->second.get();
    }

    std::size_t per_vertex_size = 3 * sizeof(float);
    if (mesh->hasNormals())
        per_vertex_size += 3 * sizeof(float);
    if (mesh->hasTexCoords())
        per_vertex_size += 2 * sizeof(float);
    if (mesh->hasTangents())
        per_vertex_size += 3 * sizeof(float);
    if (mesh->hasVertexColors())
        per_vertex_size +=  3 * sizeof(float);

    GLenum index_type;
    std::size_t per_index_size;
    if (mesh->num_vertices < 256) {
        per_index_size = sizeof(GLubyte);
        index_type = GL_UNSIGNED_BYTE;
    } else if (mesh->num_vertices < 65535) {
        per_index_size = sizeof(GLushort);
        index_type = GL_UNSIGNED_SHORT;
    } else {
        per_index_size = sizeof(GLuint);
        index_type = GL_UNSIGNED_INT;
    }

    const std::size_t size = per_vertex_size * mesh->num_vertices +
            per_index_size * mesh->num_indices;

    const auto offset = m_vertex_buffer.alloc(size, 4);

    // Upload data to GPU
    float* data = static_cast<float*>(m_vertex_buffer.offsetToPointer(offset));
    for (unsigned int i = 0; i < mesh->num_vertices; ++i) {
        *data++ = mesh->vertices[i].x;
        *data++ = mesh->vertices[i].y;
        *data++ = mesh->vertices[i].z;

        // order is specified in shader::MeshStruct (shader_interface.h)
        if (mesh->hasNormals()) {
            *data++ = mesh->normals[i].x;
            *data++ = mesh->normals[i].y;
            *data++ = mesh->normals[i].z;
        }
        if (mesh->hasTexCoords()) {
            *data++ = mesh->texcoords[i].x;
            *data++ = mesh->texcoords[i].y;
        }
        if (mesh->hasTangents()) {
            *data++ = mesh->tangents[i].x;
            *data++ = mesh->tangents[i].y;
            *data++ = mesh->tangents[i].z;
        }
        if (mesh->hasVertexColors()) {
            *data++ = mesh->vertex_colors[i].x;
            *data++ = mesh->vertex_colors[i].y;
            *data++ = mesh->vertex_colors[i].z;
        }
    }
    GLubyte* indices = reinterpret_cast<GLubyte*>(data);
    GLvoid* const index_ptr = static_cast<GLvoid*>(indices);
    for (unsigned int i = 0; i < mesh->num_indices; ++i) {
        if (per_index_size == sizeof(GLubyte)) {
            *indices = static_cast<GLubyte>(mesh->indices[i]);
        } else if (per_index_size == sizeof(GLushort)) {
            GLushort* ptr = reinterpret_cast<GLushort*>(indices);
            *ptr = static_cast<GLushort>(mesh->indices[i]);
        } else {
            GLuint* ptr = reinterpret_cast<GLuint*>(indices);
            *ptr = static_cast<GLuint>(mesh->indices[i]);
        }
        indices += per_index_size;
    }

    // Create Mesh object on GPU
    const auto mesh_offset = m_mesh_buffer.alloc();
    auto* mesh_info = reinterpret_cast<shader::MeshStruct*>(m_mesh_buffer.offsetToPointer(mesh_offset));
    mesh_info->first = static_cast<unsigned int>(offset);
    mesh_info->stride = static_cast<unsigned int>(per_vertex_size);
    mesh_info->components[0] = mesh->hasNormals();
    mesh_info->components[1] = mesh->hasTexCoords();
    mesh_info->components[2] = mesh->hasTangents();
    mesh_info->components[3] = mesh->hasVertexColors();


    auto mesh_index = mesh_offset / static_cast<GLintptr>(sizeof(shader::MeshStruct));
    auto res = m_meshes.emplace(std::move(name),
            std::unique_ptr<Mesh>(
                new Mesh(GL_TRIANGLES, index_ptr,
                static_cast<GLsizei>(mesh->num_indices), index_type,
                mesh_index, offset)));
    return res.first->second.get();
}

/****************************************************************************/

Mesh* MeshManager::getMesh(const char* name)
{
    auto it = m_meshes.find(name);
    if (it == m_meshes.end()) {
        LOG_ERROR("Can't find mesh: ", name);
        return nullptr;
    }
    return it->second.get();
}

/****************************************************************************/

const Mesh* MeshManager::getMesh(const char* name) const
{
    auto it = m_meshes.find(name);
    if (it == m_meshes.end()) {
        LOG_ERROR("Can't find mesh: ", name);
        return nullptr;
    }
    return it->second.get();
}

/****************************************************************************/

} // namespace core
