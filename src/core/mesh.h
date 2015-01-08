#ifndef CORE_MESH_H
#define CORE_MESH_H

#include "gl/gl_sys.h"
#include "util/bitfield.h"
#include "aabb.h"

namespace core
{

enum class MeshComponents : unsigned char
{
    TexCoords = 1<<0,
    Normals   = 1<<1,
    Tangents  = 1<<2
};

class Mesh
{
public:
    util::bitfield<MeshComponents> components() const;
    GLenum type() const;
    GLenum mode() const;
    GLvoid* indices() const;
    GLsizei count() const;
    GLint   basevertex() const;
    const AABB& bbox() const;
    GLuint index() const;

private:
    friend class MeshManager;

    Mesh(GLenum mode, GLsizei count, GLenum type, GLvoid* indices,
            GLint basevertex, util::bitfield<MeshComponents> components,
            const AABB& bbox, GLuint index);

    GLenum      m_mode;
    GLvoid*     m_indices;
    GLsizei     m_count;
    GLenum      m_type;
    GLint       m_basevertex;
    util::bitfield<MeshComponents> m_components;
    AABB        m_bbox;
    GLuint      m_index;
};

} // namespace core

#endif // CORE_MESH_H

