#ifndef CORE_PROGRAM_PIPELINE_H
#define CORE_PROGRAM_PIPELINE_H

#include <cstddef>
#include "gl/gl_objects.h"

namespace core
{

class ProgramPipeline
{
public:
    operator const gl::ProgramPipeline&() const;
private:
    friend class ShaderManager;
    ProgramPipeline(std::size_t idx);
    std::size_t m_idx;
};

} // namespace core

#endif // CORE_PROGRAM_PIPELINE_H

