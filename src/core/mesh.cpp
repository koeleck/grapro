#include "mesh.h"

namespace core
{

/****************************************************************************/

Mesh::Mesh(GLenum mode_, GLsizei count_, GLenum type_, GLvoid* indices_,
            GLint basevertex_, util::bitfield<MeshComponents> components_)
  : m_mode{mode_},
    m_indices{indices_},
    m_count{count_},
    m_type{type_},
    m_basevertex{basevertex_},
    m_components{components_}
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

} // namespace core
