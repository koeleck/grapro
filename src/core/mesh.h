#ifndef CORE_MESH_H
#define CORE_MESH_H

#include "gl/gl_sys.h"

namespace core
{

class Mesh
{
public:

private:
    friend class MeshManager;

    Mesh(GLenum, GLvoid*, GLsizei, GLenum, GLintptr, GLintptr);

    GLenum      m_mode;
    GLvoid*     m_indices;
    GLsizei     m_count;
    GLenum      m_type;

    GLintptr    m_index;
    GLintptr    m_offset;
};

} // namespace core

#endif // CORE_MESH_H

