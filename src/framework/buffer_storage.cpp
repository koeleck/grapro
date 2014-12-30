#include <cstdlib>
#include <algorithm>
#include "buffer_storage.h"

/****************************************************************************/

namespace
{
// from LLVM's libc++ (because gcc's libstdc++ doesn't provide std::align ... stupid gcc)
void* align(size_t alignment, size_t size, void*& ptr, size_t& space)
{
    void* r = nullptr;
    if (size <= space)
    {
        char* p1 = static_cast<char*>(ptr);
        char* p2 = reinterpret_cast<char*>((reinterpret_cast<size_t>(p1) + alignment - 1) & -alignment);
        size_t d = static_cast<size_t>(p2 - p1);
        if (d <= space - size)
        {
            r = p2;
            ptr = r;
            space -= d;
        }
    }
    return r;
}
}

namespace framework
{

/****************************************************************************/

struct BufferStorage::Segment
{
    Segment(std::size_t b, std::size_t p, std::size_t e)
      : begin{b}, ptr{p}, end{e} {}
    std::size_t begin;
    std::size_t ptr;
    std::size_t end;
};

/****************************************************************************/

BufferStorage::BufferStorage(const GLenum target, const std::size_t size,
        const GLbitfield flags)
  : m_size{size},
    m_persistent_ptr{nullptr}
{
    glBindBuffer(target, m_buffer);

    glBufferStorage(target, size, nullptr, flags);

    if (flags & GL_MAP_PERSISTENT_BIT) {
        GLbitfield map_flags = GL_MAP_PERSISTENT_BIT;
        if (flags & GL_MAP_READ_BIT)
            map_flags |= GL_MAP_READ_BIT;
        if (flags & GL_MAP_WRITE_BIT)
            map_flags |= GL_MAP_WRITE_BIT;
        if (flags & GL_MAP_COHERENT_BIT)
            map_flags |= GL_MAP_COHERENT_BIT;
        m_persistent_ptr = glMapBufferRange(target, 0, size, map_flags);
    }

    m_freelist.emplace_back(0, 0, size);
}

/****************************************************************************/

GLintptr BufferStorage::alloc(const std::size_t size, const std::size_t alignment)
{
    GLintptr offset = 0;
    std::vector<Segment>::iterator it;
    const auto it_end = m_freelist.end();
    for (it = m_freelist.begin(); it != it_end; ++it) {
        std::size_t space = it->end - it->begin;
        if (space < size)
            continue;

        void* ptr = reinterpret_cast<void*>(it->begin);
        void* res = align(alignment, size, ptr, space);

        // first fit
        if (res != nullptr) {
            const std::size_t start = reinterpret_cast<std::size_t>(ptr);

            Segment used(it->begin, start, start + size);

            if (space == size) {
                m_freelist.erase(it);
            } else {
                it->begin = used.end;
                it->ptr = used.end;
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
    std::size_t ptr = static_cast<std::size_t>(offset);

    auto it = std::find_if(m_used.begin(), m_used.end(),
            [ptr] (const Segment& seg) -> bool
            {
                return seg.ptr == ptr;
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

bool BufferStorage::hasPersistentMapping() const
{
    return m_persistent_ptr != nullptr;
}

/****************************************************************************/

void* BufferStorage::baseAddress() const
{
    return m_persistent_ptr;
}

/****************************************************************************/

void BufferStorage::clear()
{
    m_used.clear();
    m_freelist.clear();
    m_freelist.emplace_back(0, 0, m_size);
}

/****************************************************************************/

} // namespace framework

