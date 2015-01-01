#include "program.h"
#include "gl/program.h"
#include "shader_manager.h"


namespace core
{

/****************************************************************************/

Program::operator const gl::Program &() const
{
    return res::shaders->getProgram(m_idx);
}

/****************************************************************************/

Program::Program(const std::size_t idx)
  : m_idx{idx}
{
}

/****************************************************************************/

} // namespace core
