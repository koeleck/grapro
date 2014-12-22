#ifndef GL_OBJECTS_H
#define GL_OBJECTS_H

#include "gl_sys.h"
#include "nogen.h"

namespace gl
{

/*************************************************************************
 *
 * Generic class for misc. OpenGL objects
 *
 ************************************************************************/

namespace detail
{

template <typename GLAlloc>
class GLObject
{
public:
    using type = typename GLAlloc::type;

    // no copying
    GLObject(const GLObject&) = delete;
    GLObject& operator=(const GLObject&) = delete;

    GLObject() noexcept
    {
        GLAlloc::gen(1, &m_handle);
    }

    explicit GLObject(detail::DontGenerateGLObject) noexcept
      : m_handle{0}
    {
    }

    ~GLObject() noexcept
    {
        if (m_handle != 0) {
            GLAlloc::del(1, &m_handle);
            m_handle = 0;
        }
    }

    GLObject(GLObject&& other) noexcept
      : m_handle{other.m_handle}
    {
        other.m_handle = 0;
    }

    GLObject& operator=(GLObject&& other) & noexcept
    {
        swap(other);
        return *this;
    }

    void swap(GLObject& other) noexcept
    {
        const auto tmp{m_handle};
        m_handle = other.m_handle;
        other.m_handle = tmp;
    }

    explicit operator bool() const noexcept
    {
        return m_handle != 0;
    }

    bool operator!() const noexcept
    {
        return m_handle == 0;
    }

    operator type() const noexcept
    {
        return m_handle;
    }

    type get() const noexcept
    {
        return m_handle;
    }

private:

    type            m_handle;
};


/*************************************************************************
 *
 * Each struct provides methods for generateing/deleting objects
 *
 ************************************************************************/

struct BufferAlloc {
    using type = GLuint;

    static void gen(const GLsizei n, GLuint* const buffers) {
        glGenBuffers(n, buffers);
    }
    static void del(const GLsizei n, const GLuint* const buffers) {
        glDeleteBuffers(n, buffers);
    }
};

struct TextureAlloc {
    using type = GLuint;

    static void gen(const GLsizei n, GLuint* const textures) {
        glGenTextures(n, textures);
    }
    static void del(const GLsizei n, const GLuint* const textures) {
        glDeleteTextures(n, textures);
    }
};

struct QueryAlloc {
    using type = GLuint;

    static void gen(const GLsizei n, GLuint* const ids) {
        glGenQueries(n, ids);
    }
    static void del(const GLsizei n, const GLuint* const ids) {
        glDeleteQueries(n, ids);
    }
};

struct VertexArrayAlloc {
    using type = GLuint;

    static void gen(const GLsizei n, GLuint* const arrays) {
        glGenVertexArrays(n, arrays);
    }
    static void del(const GLsizei n, const GLuint* const arrays) {
        glDeleteVertexArrays(n, arrays);
    }
};

struct FramebufferAlloc {
    using type = GLuint;

    static void gen(const GLsizei n, GLuint* const ids) {
        glGenFramebuffers(n, ids);
    }
    static void del(const GLsizei n, const GLuint* const ids) {
        glDeleteFramebuffers(n, ids);
    }
};

struct RenderbufferAlloc {
    using type = GLuint;

    static void gen(const GLsizei n, GLuint* const ids) {
        glGenRenderbuffers(n, ids);
    }
    static void del(const GLsizei n, const GLuint* const ids) {
        glDeleteRenderbuffers(n, ids);
    }
};

struct SamplerAlloc {
    using type = GLuint;

    static void gen(const GLsizei n, GLuint* const ids) {
        glGenSamplers(n, ids);
    }
    static void del(const GLsizei n, const GLuint* const ids) {
        glDeleteSamplers(n, ids);
    }
};

struct ProgramPipelineAlloc {
    using type = GLuint;

    static void gen(const GLsizei n, GLuint* const ids) {
        glGenProgramPipelines(n, ids);
    }
    static void del(const GLsizei n, const GLuint* const ids) {
        glDeleteProgramPipelines(n, ids);
    }
};

struct TransformFeedbackAlloc {
    using type = GLuint;

    static void gen(const GLsizei n, GLuint* const ids) {
        glGenTransformFeedbacks(n, ids);
    }
    static void del(const GLsizei n, const GLuint* const ids) {
        glDeleteTransformFeedbacks(n, ids);
    }
};

} // namespace detail


/*************************************************************************
 *
 * Typedefs for your convenience
 *
 ************************************************************************/
using Buffer            = detail::GLObject<detail::BufferAlloc>;
using VertexArray       = detail::GLObject<detail::VertexArrayAlloc>;
using Texture           = detail::GLObject<detail::TextureAlloc>;
using Query             = detail::GLObject<detail::QueryAlloc>;
using Framebuffer       = detail::GLObject<detail::FramebufferAlloc>;
using Renderbuffer      = detail::GLObject<detail::RenderbufferAlloc>;
using Sampler           = detail::GLObject<detail::SamplerAlloc>;
using ProgramPipeline   = detail::GLObject<detail::ProgramPipelineAlloc>;
using TransformFeedback = detail::GLObject<detail::TransformFeedbackAlloc>;

} // namespace gl

#endif // GL_OBJECTS_H
