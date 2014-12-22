#include <stdexcept>

#include "program.h"
#include "log/log.h"

namespace gl
{

////////////////////////////////////////////////////////////////////////////////////

Program linkProgram(const std::initializer_list<std::reference_wrapper<const Shader>> shaders)
{
    Program p;
    for (const auto& s : shaders) {
        glAttachShader(p, s.get());
    }
    glLinkProgram(p);
    for (const auto& s : shaders) {
        glDetachShader(p, s.get());
    }

    if (!p) {
        LOG_ERROR(logtag::OpenGL, "Failed to link program:\n",
                p.getInfoLog());
        throw std::runtime_error("Failed to link program");
    }

    return p;
}

////////////////////////////////////////////////////////////////////////////////////

std::string Program::getInfoLog() const
{
    if (m_handle == 0)
        return "";

    GLint length;
    glGetProgramiv(m_handle, GL_INFO_LOG_LENGTH, &length);
    if (length == 0)
        return "";

    GLint tmp;
    std::string log(static_cast<size_t>(length), '\0');
    glGetProgramInfoLog(m_handle, length, &tmp, &log[0]);
    // pop '\0' from back of log.
    log.pop_back();
    return log;
}

////////////////////////////////////////////////////////////////////////////////////

} // namespace gl
