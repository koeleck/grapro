#ifndef CORE_BUFFER_STORAGE_POOL_H
#define CORE_BUFFER_STORAGE_POOL_H

#include "gl/gl_objects.h"
#include "log/log.h"
#include <algorithm>
#include <vector>
#include <utility>

namespace core
{

template <typename T>
class BufferStoragePool
{
private:
    static constexpr int first_offset() {return (4 > T::alignment()) ? 4 : T::alignment();}

public:
    BufferStoragePool(GLenum target, std::size_t cap)
      : m_capacity{cap},
        m_size{0}
    {
        static_assert((sizeof(T) % T::alignment()) == 0, "");

        auto buffer_size = m_capacity * sizeof(T);

        // first 4 bytes are for the counter
        m_freelist.emplace_back(first_offset(), buffer_size);

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
        seg.first += sizeof(T);
        if (seg.first == seg.second)
            m_freelist.pop_back();
        m_size++;
        *static_cast<int*>(m_data) = static_cast<int>(m_size);
        return res;
    }

    void free(GLintptr offset)
    {
        // TODO
        m_freelist.emplace_back(offset, offset + sizeof(T));
        m_size--;
        *static_cast<int*>(m_data) = static_cast<int>(m_size);
    }


    T* offsetToPointer(const GLintptr offset) const
    {
        const auto tmp = static_cast<char*>(m_data);
        return reinterpret_cast<T*>(tmp + offset);
    }

    GLuint offsetToIndex(const GLintptr offset) const
    {
        return static_cast<GLuint>(offset - first_offset()) /
                static_cast<GLuint>(sizeof(T));
    }

    GLintptr pointerToOffset(const T* const ptr) const
    {
        return reinterpret_cast<const char*>(ptr) - static_cast<const char*>(m_data);
    }

    GLintptr indexToOffset(const GLuint index) const
    {
        return (static_cast<GLintptr>(index) * static_cast<GLintptr>(sizeof(T)))
            + first_offset();
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

