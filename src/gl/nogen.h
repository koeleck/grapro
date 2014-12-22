#ifndef GL_NOGEN_HPP
#define GL_NOGEN_HPP

namespace gl
{

namespace detail
{

enum class DontGenerateGLObject : char
{
    NO_GEN
};

} // namespace detail

constexpr detail::DontGenerateGLObject NO_GEN = detail::DontGenerateGLObject::NO_GEN;

} // namespace gl

#endif // GL_NOGEN_HPP
