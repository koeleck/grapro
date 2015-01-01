#ifndef CORE_PROGRAM_H
#define CORE_PROGRAM_H

#include <cstddef>

namespace gl
{
class Program;
}

namespace core
{

class Program
{
public:
    operator const gl::Program&() const;
private:
    friend class ShaderManager;
    Program(std::size_t idx);
    std::size_t m_idx;
};

} // namespace core

#endif // CORE_PROGRAM_H
