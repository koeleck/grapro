#ifndef IMPORT_MESH_H
#define IMPORT_MESH_H

#include <cstdint>
#include <glm/glm.hpp>

#include "core/aabb.h"

namespace import
{

struct Mesh
{
    static constexpr int VERSION = 2;

    void makePointersRelative();
    void makePointersAbsolute();

    const char*     name;
    std::uint32_t*  indices;
    glm::vec3*      vertices;
    glm::vec3*      normals;
    glm::vec4*      tangents;
    glm::vec3*      vertex_colors;
    glm::vec2*      texcoords;
    core::AABB      bbox;
    std::uint32_t   material_index;
    std::uint32_t   num_vertices;
    std::uint32_t   num_indices;

    bool hasNormals() const noexcept
    {
        return normals != nullptr;
    }

    bool hasTangents() const noexcept
    {
        return tangents != nullptr;
    }

    bool hasTexCoords() const noexcept
    {
        return texcoords != nullptr;
    }

    bool hasVertexColors() const noexcept
    {
        return vertex_colors != nullptr;
    }
};

} // namespace import

#endif // IMPORT_MESH_H

