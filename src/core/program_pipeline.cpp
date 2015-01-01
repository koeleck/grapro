#include "program_pipeline.h"
#include "gl/gl_objects.h"
#include "shader_manager.h"


namespace core
{

/****************************************************************************/

ProgramPipeline::operator const gl::ProgramPipeline &() const
{
    return res::shaders->getProgramPipeline(m_idx);
}

/****************************************************************************/

ProgramPipeline::ProgramPipeline(const std::size_t idx)
  : m_idx{idx}
{
}

/****************************************************************************/

} // namespace core
