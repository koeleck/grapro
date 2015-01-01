#ifndef CORE_SHADER_H
#define CORE_SHADER_H

#include <cstddef>

namespace gl
{
class Shader;
}

namespace core
{

class Shader
{
public:
    operator const gl::Shader&() const;
private:
    friend class ShaderManager;
    Shader(std::size_t idx);
    std::size_t m_idx;
};

} // namespace core

#endif // CORE_SHADER_H

