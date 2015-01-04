#include "mesh.h"

namespace core
{

/****************************************************************************/

Mesh::Mesh(const GLenum mode_, GLvoid* const indices, const GLsizei count,
        const GLenum type_, const GLuint index, const GLintptr offset)
  : m_mode{mode_},
    m_indices{indices},
    m_count{count},
    m_type{type_},
    m_index{index},
    m_offset{offset}
{
}

/****************************************************************************/

GLuint Mesh::getIndex() const
{
    return m_index;
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

GLvoid* Mesh::indirect() const
{
    return m_indices;
}

/****************************************************************************/

} // namespace core
