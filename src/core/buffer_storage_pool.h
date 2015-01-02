#ifndef CORE_BUFFER_STORAGE_POOL_H
#define CORE_BUFFER_STORAGE_POOL_H

#include "gl/gl_objects.h"
#include "log/log.h"
#include <vector>
#include <utility>

namespace core
{

template <std::size_t element_size>
class BufferStoragePool
{
public:
    BufferStoragePool(GLenum target, std::size_t cap)
      : m_capacity{cap},
        m_size{0}
    {
        auto buffer_size = m_capacity * element_size;
        m_freelist.emplace_back(0, buffer_size);

        glBindBuffer(target, m_buffer);

        glBufferStorage(target, buffer_size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
        m_data = glMapBufferRange(target, 0, buffer_size,
                GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    }

    GLintptr alloc()
    {
        if (m_freelist.empty()) {
            LOG_ERROR("BufferStoragePool is full");
            abort();
        }
        auto& seg = m_freelist.back();
        const auto res = seg.first;
        seg.first += element_size;
        if (seg.first == seg.second)
            m_freelist.pop_back();
        m_size++;
        return res;
    }

    void free(GLintptr offset)
    {
        // TODO
        m_freelist.emplace_back(offset, offset + element_size);
        m_size--;
    }


    void* offsetToPointer(const GLintptr offset) const
    {
        const auto tmp = static_cast<char*>(m_data);
        return static_cast<void*>(tmp + offset);
    }


    GLintptr pointerToOffset(const void* const ptr) const
    {
        return static_cast<const char*>(ptr) - static_cast<const char*>(m_data);
    }

    std::size_t capacity() const
    {
        return m_capacity;
    }

    std::size_t size() const
    {
        return m_size;
    }

    const gl::Buffer& buffer() const
    {
        return m_buffer;
    }

/****************************************************************************/

private:

    gl::Buffer      m_buffer;
    std::size_t     m_capacity;
    void*           m_data;
    std::size_t     m_size;

    std::vector<std::pair<GLintptr, GLintptr>>    m_freelist;

};

} // namespace core

#endif // CORE_BUFFER_STORAGE_POOL_H

