#include "mesh.h"

namespace core
{

/****************************************************************************/

Mesh::Mesh(GLenum mode_, GLsizei count_, GLenum type_, GLvoid* indices_,
            GLint basevertex_, util::bitfield<MeshComponents> components_,
            const AABB& bbox_, const GLuint index_)
  : m_mode{mode_},
    m_indices{indices_},
    m_count{count_},
    m_type{type_},
    m_basevertex{basevertex_},
    m_components{components_},
    m_bbox{bbox_},
    m_index{index_}
{
}

/****************************************************************************/

GLenum Mesh::type() const
{
    return m_type;
}

/****************************************************************************/

GLenum Mesh::mode() const
{
    return m_mode;
}

/****************************************************************************/

GLvoid* Mesh::indices() const
{
    return m_indices;
}

/****************************************************************************/

GLsizei Mesh::count() const
{
    return m_count;
}

/****************************************************************************/

util::bitfield<MeshComponents> Mesh::components() const
{
    return m_components;
}

/****************************************************************************/

GLint Mesh::basevertex() const
{
    return m_basevertex;
}

/****************************************************************************/

const AABB& Mesh::bbox() const
{
    return m_bbox;
}

/****************************************************************************/

GLuint Mesh::index() const
{
    return m_index;
}

/****************************************************************************/

GLuint Mesh::firstIndex() const
{
    unsigned int size;
    if (m_type == GL_UNSIGNED_BYTE)
        size = 1;
    else if (m_type == GL_UNSIGNED_SHORT)
        size = 2;
    else
        size = 4;
    return static_cast<GLuint>(reinterpret_cast<std::size_t>(m_indices) / size);
}

/****************************************************************************/

} // namespace core
