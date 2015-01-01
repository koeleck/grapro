#ifndef GL_SYS_H
#define GL_SYS_H

#if !defined(GLEW_NO_GLU)
#define GLEW_NO_GLU
#endif
#include <GL/glew.h>
#include <string>

namespace gl
{

bool initGL();
std::string enumToString(GLenum e);
GLenum stringToEnum(const std::string& s);


} // namespace gl

#endif // GL_SYS_H

