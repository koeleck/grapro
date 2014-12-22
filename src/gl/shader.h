#ifndef GL_SHADER_H
#define GL_SHADER_H

#include <string>
#include "gl_sys.h"

namespace gl
{

class Shader
{
public:

    // no copying
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader() noexcept : m_handle{0} {}

    ~Shader() noexcept {
        if (m_handle != 0) {
            glDeleteShader(m_handle);
            m_handle = 0;
        }
    }

    Shader(const GLenum type) noexcept
      : m_handle{glCreateShader(type)}
    {
    }

    Shader(Shader&& other) noexcept
      : m_handle{other.m_handle} {
        other.m_handle = 0;
    }

    Shader& operator=(Shader&& other) & noexcept {
        swap(other);
        return *this;
    }

    void swap(Shader& other) noexcept {
        const auto tmp = m_handle;
        m_handle = other.m_handle;
        other.m_handle = tmp;
    }

    std::string getInfoLog() const;

    bool compileStatus() const noexcept {
        GLint result;
        glGetShaderiv(m_handle, GL_COMPILE_STATUS, &result);
        return result == GL_TRUE;
    }

    operator GLuint() const noexcept {
        return m_handle;
    }

    //! **true** if shader compiles successfully
    explicit operator bool() const noexcept {
        return (m_handle != 0) && compileStatus();
    }

    bool operator!() const noexcept
    {
        return m_handle == 0 || !compileStatus();
    }

    GLuint get() const noexcept {
        return m_handle;
    }

private:
    GLuint      m_handle;
};

// helper function for loading shaders
Shader loadShader(GLenum type, const std::string& filename);

} // namespace gl

#endif // GL_SHADER_H
