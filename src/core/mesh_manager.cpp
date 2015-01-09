#include <cassert>

#include "mesh_manager.h"
#include "import/mesh.h"
#include "log/log.h"
#include "framework/vars.h"

namespace core
{

/****************************************************************************/

constexpr int MAX_NUM_MESHES = 1200;
MeshManager::MeshManager()
  : m_data(GL_SHADER_STORAGE_BUFFER, vars.vertex_buffer_size),
    m_mesh_pool(GL_SHADER_STORAGE_BUFFER, MAX_NUM_MESHES)
{
    initVAOs();
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

    // observe the order in 'shader_interface.h'
    util::bitfield<MeshComponents> components;
    GLsizeiptr per_vertex_size = 3 * sizeof(float);
    if (mesh->hasTexCoords()) {
        per_vertex_size += 2 * sizeof(float);
        components |= MeshComponents::TexCoords;
    }
    // if mesh has tangents, then use we use tangent
    // and bitangent plus a reflect float to reconstruct
    // the normal
    if (mesh->hasTangents()) {
        per_vertex_size += 7 * sizeof(float);
        components |= MeshComponents::Tangents;
    } else if (mesh->hasNormals()) {
        per_vertex_size += 3 * sizeof(float);
        components |= MeshComponents::Normals;
    }

    GLenum index_type;
    GLsizeiptr per_index_size;
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

    const GLsizeiptr vertices_size = per_vertex_size * mesh->num_vertices;
    const GLsizeiptr indices_size = per_index_size * mesh->num_indices;
    const GLsizeiptr size = vertices_size + indices_size;

    // allocate memory and make sure the first address is aligned to
    // a multiple of the per_vertex_size
    const auto offset = m_data.alloc(size, static_cast<GLsizei>(vertices_size));
    assert(offset % vertices_size == 0);

    // Upload data to GPU
    void* ptr = glMapNamedBufferRangeEXT(m_data.buffer(),
                offset, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
    float* data = static_cast<float*>(ptr);
    for (unsigned int i = 0; i < mesh->num_vertices; ++i) {
        *data++ = mesh->vertices[i][0];
        *data++ = mesh->vertices[i][1];
        *data++ = mesh->vertices[i][2];

        // order is specified in shader::MeshStruct (shader_interface.h)
        if (mesh->hasTexCoords()) {
            *data++ = mesh->texcoords[i][0];
            *data++ = mesh->texcoords[i][1];
        }
        if (mesh->hasTangents()) {
            *data++ = mesh->tangents[i][0];
            *data++ = mesh->tangents[i][1];
            *data++ = mesh->tangents[i][2];
            *data++ = mesh->bitangents[i][0];
            *data++ = mesh->bitangents[i][1];
            *data++ = mesh->bitangents[i][2];

            *data++ = (glm::dot(mesh->normals[i],
                    glm::cross(mesh->tangents[i], mesh->bitangents[i])) > .0f) ?
                    1.f : -1.f;
        } else if (mesh->hasNormals()) {
            *data++ = mesh->normals[i][0];
            *data++ = mesh->normals[i][1];
            *data++ = mesh->normals[i][2];
        }
        if (mesh->hasVertexColors()) {
            *data++ = mesh->vertex_colors[i][0];
            *data++ = mesh->vertex_colors[i][1];
            *data++ = mesh->vertex_colors[i][2];
        }
    }
    // since we're only pushing floats into the buffer,
    // the start of out index array is always aligned to
    // 4 bytes... good
    GLubyte* indices = reinterpret_cast<GLubyte*>(data);
    for (unsigned int i = 0; i < mesh->num_indices; ++i) {
        if (per_index_size == sizeof(GLubyte)) {
            *indices = static_cast<GLubyte>(mesh->indices[i]);
        } else if (per_index_size == sizeof(GLushort)) {
            GLushort* idx = reinterpret_cast<GLushort*>(indices);
            *idx = static_cast<GLushort>(mesh->indices[i]);
        } else {
            GLuint* idx = reinterpret_cast<GLuint*>(indices);
            *idx = static_cast<GLuint>(mesh->indices[i]);
        }
        indices += per_index_size;
    }
    assert(indices - static_cast<GLubyte*>(ptr) == size);
    glUnmapNamedBufferEXT(m_data.buffer());

    // Add MeshStruct on GPU
    const GLintptr mesh_offset = m_mesh_pool.alloc();
    const GLuint mesh_index = static_cast<GLuint>(mesh_offset /
            static_cast<GLintptr>(sizeof(shader::MeshStruct)));
    auto* mesh_data = static_cast<shader::MeshStruct*>(m_mesh_pool.offsetToPointer(mesh_offset));
    // remember: we're using a float[] array, so divide everything by sizeof(float)
    mesh_data->stride = static_cast<GLuint>(per_vertex_size / static_cast<GLsizei>(sizeof(float)));
    mesh_data->components = static_cast<GLuint>(components());
    mesh_data->first = static_cast<GLuint>(offset / static_cast<GLintptr>(sizeof(float)));

    auto res = m_meshes.emplace(std::move(name),
            std::unique_ptr<Mesh>(
                new Mesh(GL_TRIANGLES,
                static_cast<GLsizei>(mesh->num_indices),
                index_type,
                reinterpret_cast<GLvoid*>(offset + vertices_size),
                static_cast<GLint>(offset / per_vertex_size),
                components,
                mesh->bbox,
                mesh_index)));
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

void MeshManager::initVAOs()
{
    for (unsigned char i = 0; i < 8; ++i) {
        util::bitfield<MeshComponents> c(i);
        GLsizei stride = static_cast<GLsizei>(3 * sizeof(float));
        if (c & MeshComponents::TexCoords)
            stride += static_cast<GLsizei>(2 * sizeof(float));
        if (c & MeshComponents::Normals)
            stride += static_cast<GLsizei>(3 * sizeof(float));
        if (c & MeshComponents::Tangents)
            stride += static_cast<GLsizei>(7 * sizeof(float));

        gl::VertexArray vao;
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_data.buffer());

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);

        // observe the order in 'shader_interface.h'
        auto offset = 3 * sizeof(float);
        if (c & MeshComponents::TexCoords) {
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
                    reinterpret_cast<GLvoid*>(offset));
            offset += 2 * sizeof(float);
        }
        if (c & MeshComponents::Normals) {
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, stride,
                    reinterpret_cast<GLvoid*>(offset));
            offset += 3 * sizeof(float);
        }
        if (c & MeshComponents::Tangents) {
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_TRUE, stride,
                    reinterpret_cast<GLvoid*>(offset));
            offset += 3 * sizeof(float);
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 4, GL_FLOAT, GL_TRUE, stride,
                    reinterpret_cast<GLvoid*>(offset));
            offset += 4 * sizeof(float);
        }

        assert(static_cast<GLsizei>(offset) == stride);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_data.buffer());
        glBindVertexArray(0);

        m_vaos.emplace(c(), std::move(vao));
    }
}

/****************************************************************************/

GLuint MeshManager::getVAO(const Mesh* mesh) const
{
    if (mesh == nullptr)
        return 0;
    auto it = m_vaos.find(mesh->components()());
    if (it == m_vaos.end()) {
        LOG_ERROR("No suitable VAO found");
        return 0;
    }
    return it->second.get();
}

/****************************************************************************/

void MeshManager::bind() const
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindings::VERTEX,
            m_data.buffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindings::MESH,
            m_mesh_pool.buffer());
}

/****************************************************************************/

GLuint MeshManager::getElementArrayBuffer() const
{
    return m_data.buffer().get();
}

/****************************************************************************/

} // namespace core
