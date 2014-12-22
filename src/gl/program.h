#ifndef GL_PROGRAM_H
#define GL_PROGRAM_H

#include <initializer_list>
#include <string>
#include <cassert>
#include <functional>

#include "gl_sys.h"
#include "shader.h"
#include "nogen.h"

namespace gl
{

class Program
{
public:
    // no copying
    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;

    Program() noexcept
      : m_handle{glCreateProgram()}
    {
    }

    explicit Program(detail::DontGenerateGLObject) noexcept
      : m_handle{0}
    {
    }

    ~Program() noexcept
    {
        if (m_handle != 0) {
            glDeleteProgram(m_handle);
            m_handle = 0;
        }
    }

    Program(Program&& other) noexcept
      : m_handle{other.m_handle}
    {
        other.m_handle = 0;
    }

    Program& operator=(Program&& other) & noexcept
    {
        swap(other);
        return *this;
    }

    void swap(Program& other) noexcept
    {
        const auto tmp = m_handle;
        m_handle = other.m_handle;
        other.m_handle = tmp;
    }

    bool linkStatus() const noexcept
    {
        GLint result;
        glGetProgramiv(m_handle, GL_LINK_STATUS, &result);
        return result == GL_TRUE;
    }

    bool valid() const noexcept
    {
        GLint result;
        glGetProgramiv(m_handle, GL_VALIDATE_STATUS, &result);
        return result == GL_TRUE;
    }

    std::string getInfoLog() const;

    operator GLuint() const noexcept
    {
        return m_handle;
    }

    GLuint get() const noexcept
    {
        return m_handle;
    }

    explicit operator bool() const noexcept
    {
        return (m_handle != 0) && linkStatus();
    }

    bool operator!() const noexcept
    {
        return m_handle == 0 || !linkStatus();
    }

private:
    GLuint      m_handle;
};

// helper function for creating a program
Program linkProgram(std::initializer_list<std::reference_wrapper<const Shader>> shaders);

} // namespace gl

#endif // GL_PROGRAM_H
