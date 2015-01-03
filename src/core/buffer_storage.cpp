#include <cstdlib>
#include <algorithm>
#include "buffer_storage.h"

/****************************************************************************/

namespace
{
// from LLVM's libc++ (because gcc's libstdc++ doesn't provide std::align ... stupid gcc)
bool align(size_t alignment, size_t size, void*& ptr, size_t& space)
{
    bool success = false;
    if (size <= space)
    {
        char* p1 = static_cast<char*>(ptr);
        char* p2 = reinterpret_cast<char*>((reinterpret_cast<size_t>(p1) + alignment - 1) & -alignment);
        size_t d = static_cast<size_t>(p2 - p1);
        if (d <= space - size)
        {
            ptr = p2;
            space -= d;
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
    Segment(std::size_t b, std::size_t p, std::size_t e)
      : begin{b}, ptr{p}, end{e} {}
    std::size_t begin;
    std::size_t ptr;
    std::size_t end;
};

/****************************************************************************/

BufferStorage::BufferStorage(const GLenum target, const std::size_t size)
  : m_size{size},
    m_persistent_ptr{nullptr}
{
    glBindBuffer(target, m_buffer);

    glBufferStorage(target, size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);

    m_persistent_ptr = glMapBufferRange(target, 0, size,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    m_freelist.emplace_back(0, 0, size);
}

/****************************************************************************/

BufferStorage::~BufferStorage() = default;

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

        // first fit
        if (align(alignment, size, ptr, space)) {
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

const gl::Buffer& BufferStorage::buffer() const
{
    return m_buffer;
}

/****************************************************************************/

void* BufferStorage::offsetToPointer(const GLintptr offset) const
{
    const auto tmp = static_cast<char*>(m_persistent_ptr);
    return static_cast<void*>(tmp + offset);
}

/****************************************************************************/

GLintptr BufferStorage::pointerToOffset(const void* const ptr) const
{
    return static_cast<const char*>(ptr) - static_cast<const char*>(m_persistent_ptr);
}

/****************************************************************************/

} // namespace core

