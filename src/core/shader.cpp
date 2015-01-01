#include "shader.h"
#include "gl/shader.h"
#include "shader_manager.h"


namespace core
{

/****************************************************************************/

Shader::operator const gl::Shader &() const
{
    return res::shaders->getShader(m_idx);
}

/****************************************************************************/

Shader::Shader(const std::size_t idx)
  : m_idx{idx}
{
}

/****************************************************************************/

} // namespace core
