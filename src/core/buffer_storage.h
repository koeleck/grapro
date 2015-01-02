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
    BufferStorage(GLenum target, std::size_t size);
    ~BufferStorage();

    GLintptr alloc(std::size_t size, std::size_t alignment);
    void free(GLintptr offset);

    void clear();

    void* baseAddress() const;

    void* offsetToPointer(GLintptr offset) const;
    GLintptr pointerToOffset(const void* ptr) const;

    const gl::Buffer& buffer() const;

private:
    struct Segment;

    gl::Buffer              m_buffer;
    std::size_t             m_size;
    std::vector<Segment>    m_freelist;
    std::vector<Segment>    m_used;
    void*                   m_persistent_ptr;
};

} // namespace core

#endif // CORE_BUFFER_STORAGE_H

