#include <cstdlib>
#include <algorithm>
#include "buffer_storage.h"

/****************************************************************************/

namespace
{

bool align(GLsizei alignment, GLsizeiptr size, GLintptr& ptr, GLsizeiptr space)
{
    bool success = false;
    if (size <= space) {
        GLintptr ptr2 = ((ptr + alignment - 1) / alignment) * alignment;
        GLsizeiptr diff = static_cast<GLsizeiptr>(ptr2 - ptr);
        if (diff <= space - size) {
            ptr = ptr2;
            space -= diff;
            success = true;
        }
    }
    return success;
}

}

namespace core
{

/****************************************************************************/

struct BufferStorage::Segment
{
    Segment(GLintptr b, GLintptr p, GLintptr e)
      : begin{b}, ptr{p}, end{e} {}
    GLintptr begin;
    GLintptr ptr;
    GLintptr end;
};

/****************************************************************************/

BufferStorage::BufferStorage(const GLenum target, const GLsizeiptr size)
  : m_size{size}
{
    glBindBuffer(target, m_buffer);

    glBufferStorage(target, size, nullptr, GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);

    m_freelist.emplace_back(0, 0, size);
}

/****************************************************************************/

BufferStorage::~BufferStorage() = default;

/****************************************************************************/

GLintptr BufferStorage::alloc(const GLsizeiptr size, const GLsizei alignment)
{
    GLintptr offset = 0;
    std::vector<Segment>::iterator it;
    const auto it_end = m_freelist.end();
    for (it = m_freelist.begin(); it != it_end; ++it) {
        GLsizeiptr space = static_cast<GLsizeiptr>(it->end - it->begin);

        GLintptr ptr = it->begin;
        // first fit
        if (align(alignment, size, ptr, space)) {
            offset = ptr;
            Segment used(it->begin, ptr, ptr + size);

            it->begin = used.end;
            it->ptr = used.end;
            if (it->begin == it->end) {
                m_freelist.erase(it);
            }

            auto pos = std::lower_bound(m_used.begin(), m_used.end(),
                    used, [] (const Segment& s0, const Segment& s1) -> bool
                    {
                        return s0.begin < s1.begin;
                    });
            m_used.insert(pos, std::move(used));

            break;
        }
    }
    if (it == it_end)
        abort();

    return offset;
}

/****************************************************************************/

void BufferStorage::free(GLintptr offset)
{
    auto it = std::find_if(m_used.begin(), m_used.end(),
            [offset] (const Segment& seg) -> bool
            {
                return seg.ptr == offset;
            });
    if (it == m_used.end())
        abort();

    Segment new_seg(it->begin, it->begin, it->end);

    auto pos = std::lower_bound(m_freelist.begin(), m_freelist.end(), new_seg,
            [] (const Segment& s0, const Segment& s1) -> bool
            {
                return s0.begin < s1.begin;
            });
    pos = m_freelist.insert(pos, std::move(new_seg));
    std::size_t idx = static_cast<std::size_t>(
            std::distance(m_freelist.begin(), pos));

    Segment& seg = m_freelist[idx];
    if (idx > 0) {
        Segment& prev_seg = m_freelist[idx - 1];
        if (prev_seg.end == seg.begin) {
            seg.begin = prev_seg.begin;
            seg.ptr = prev_seg.ptr;
            m_freelist.erase(m_freelist.begin() +
                    static_cast<long>(idx) - 1);
            idx--;
        }
    }
    if (idx < m_freelist.size() - 1) {
        Segment& next_seg = m_freelist[idx + 1];
        if (seg.end == next_seg.begin) {
            seg.end = next_seg.end;
            m_freelist.erase(m_freelist.begin() +
                    static_cast<long>(idx) + 1);
        }
    }
}

/****************************************************************************/

void BufferStorage::clear()
{
    m_used.clear();
    m_freelist.clear();
    m_freelist.emplace_back(0, 0, m_size);
}

/****************************************************************************/

const gl::Buffer& BufferStorage::buffer() const
{
    return m_buffer;
}

/****************************************************************************/

} // namespace core

