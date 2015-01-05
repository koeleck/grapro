#ifndef CORE_BUFFER_STORAGE_H
#define CORE_BUFFER_STORAGE_H

#include "gl/gl_objects.h"
#include <cstddef>
#include <vector>

namespace core
{

class BufferStorage
{
public:
    BufferStorage(GLenum target, GLsizeiptr size);
    ~BufferStorage();

    GLintptr alloc(GLsizeiptr size, GLsizei alignment);
    void free(GLintptr offset);

    void clear();

    const gl::Buffer& buffer() const;

private:
    struct Segment;

    gl::Buffer              m_buffer;
    GLsizeiptr              m_size;
    std::vector<Segment>    m_freelist;
    std::vector<Segment>    m_used;
};

} // namespace core

#endif // CORE_BUFFER_STORAGE_H

