#include <stdexcept>
#include <fstream>
#include <sstream>

#include "log/log.h"
#include "shader.h"
#include "shadersource.h"

namespace gl
{

//////////////////////////////////////////////////////////////////////////

Shader loadShader(const GLenum type, const std::string& filename)
{

    ShaderSource source(filename);
    if (!source) {
        throw std::runtime_error("Failed to read shader file: " + filename);
    }

    Shader s(type);

    glShaderSource(s, 1, source.sourcePtr(), nullptr);
    glCompileShader(s);

    if (!s) {
        std::ostringstream msg;
        msg << "Failed to compile shader!\nFILES:\n";
        int i = 0;
        for (const auto& f : source.filenames()) {
            msg << " (" << i++ << ") " << f << '\n';
        }
        msg << "Info Log:\n" << s.getInfoLog();

        LOG_ERROR(logtag::OpenGL, msg.str());

        throw std::runtime_error("Failed to compile shader: " + filename);
    }

    return s;
}

//////////////////////////////////////////////////////////////////////////

std::string Shader::getInfoLog() const
{
    if (m_handle == 0)
        return "";

    GLint length;
    glGetShaderiv(m_handle, GL_INFO_LOG_LENGTH, &length);
    if (length == 0)
        return "";

    GLsizei tmp;
    std::string log(static_cast<size_t>(length), '\0');
    glGetShaderInfoLog(m_handle, length, &tmp,
            &log[0]);
    // pop '\0' from back:
    log.pop_back();
    return log;
}

} // namespace gl
