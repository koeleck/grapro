#include "mesh.h"

namespace core
{

/****************************************************************************/

Mesh::Mesh(const GLenum mode, GLvoid* const indices, const GLsizei count,
        const GLenum type, const GLuint index, const GLintptr offset)
  : m_mode{mode},
    m_indices{indices},
    m_count{count},
    m_type{type},
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

} // namespace core
