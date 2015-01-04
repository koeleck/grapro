#include <limits>

#include "program.h"
#include "gl/program.h"
#include "shader_manager.h"


namespace core
{

constexpr auto INVALID = std::numeric_limits<std::size_t>::max();

/****************************************************************************/

Program::Program()
  : m_idx{INVALID}
{
}

/****************************************************************************/

Program::operator GLuint() const
{
    if (m_idx != INVALID)
        return res::shaders->getProgram(m_idx).get();
    return 0;
}

/****************************************************************************/

Program::Program(const std::size_t idx)
  : m_idx{idx}
{
}

/****************************************************************************/

} // namespace core
