#ifndef CORE_PROGRAM_H
#define CORE_PROGRAM_H

#include <cstddef>
#include "gl/gl_sys.h"

namespace gl
{
class Program;
}

namespace core
{

class Program
{
public:
    Program();
    operator GLuint() const;

private:
    friend class ShaderManager;
    Program(std::size_t idx);
    std::size_t m_idx;
};

} // namespace core

#endif // CORE_PROGRAM_H
