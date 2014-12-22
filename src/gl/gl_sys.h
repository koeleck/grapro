#ifndef GL_SYS_H
#define GL_SYS_H

#if !defined(GLEW_NO_GLU)
#define GLEW_NO_GLU
#endif
#include <GL/glew.h>

namespace gl
{

bool initGL();

} // namespace gl

#endif // GL_SYS_H

