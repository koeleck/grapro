#ifndef FRAMEWORK_BUFFER_STORAGE_H
#define FRAMEWORK_BUFFER_STORAGE_H

#include "gl/gl_objects.h"
#include <cstddef>
#include <vector>

namespace framework
{

class BufferStorage
{
public:
    BufferStorage(GLenum target, std::size_t size, GLenum flags);

    GLintptr alloc(std::size_t size, std::size_t alignment);
    void free(GLintptr offset);

    void clear();

    bool hasPersistentMapping() const;
    void* baseAddress() const;

    const gl::Buffer& buffer() const;

private:
    struct Segment;

    gl::Buffer              m_buffer;
    std::size_t             m_size;
    std::vector<Segment>    m_freelist;
    std::vector<Segment>    m_used;
    void*                   m_persistent_ptr;
};

} // namespace framework

#endif // FRAMEWORK_BUFFER_STORAGE_H

