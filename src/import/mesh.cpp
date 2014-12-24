#include "mesh.h"

namespace import
{

/****************************************************************************/

void Mesh::makePointersRelative()
{
    const char* offset = reinterpret_cast<const char*>(this);
    const char* pos = reinterpret_cast<const char*>(name);

    if (pos < offset)
        return;

    name = reinterpret_cast<const char*>(pos - offset);

    pos = reinterpret_cast<const char*>(indices);
    indices = reinterpret_cast<std::uint32_t*>(pos - offset);

    pos = reinterpret_cast<const char*>(vertices);
    vertices = reinterpret_cast<glm::vec3*>(pos - offset);

    pos = reinterpret_cast<const char*>(normals);
    normals = reinterpret_cast<glm::vec3*>(pos - offset);

    pos = reinterpret_cast<const char*>(tangents);
    tangents = reinterpret_cast<glm::vec3*>(pos - offset);

    pos = reinterpret_cast<const char*>(vertex_colors);
    vertex_colors = reinterpret_cast<glm::vec3*>(pos - offset);

    pos = reinterpret_cast<const char*>(texcoords);
    texcoords = reinterpret_cast<glm::vec2*>(pos - offset);
}

/****************************************************************************/

void Mesh::makePointersAbsolute()
{
    char* offset = reinterpret_cast<char*>(this);
    std::size_t pos = reinterpret_cast<std::size_t>(name);

    if (reinterpret_cast<char*>(pos) > offset)
        return;

    name = reinterpret_cast<const char*>(offset + pos);

    pos = reinterpret_cast<std::size_t>(indices);
    indices = reinterpret_cast<std::uint32_t*>(offset + pos);

    pos = reinterpret_cast<std::size_t>(vertices);
    vertices = reinterpret_cast<glm::vec3*>(offset + pos);

    pos = reinterpret_cast<std::size_t>(normals);
    normals = reinterpret_cast<glm::vec3*>(offset + pos);

    pos = reinterpret_cast<std::size_t>(tangents);
    tangents = reinterpret_cast<glm::vec3*>(offset + pos);

    pos = reinterpret_cast<std::size_t>(vertex_colors);
    vertex_colors = reinterpret_cast<glm::vec3*>(offset + pos);

    pos = reinterpret_cast<std::size_t>(texcoords);
    texcoords = reinterpret_cast<glm::vec2*>(offset + pos);
}

/****************************************************************************/

} // namespace import


