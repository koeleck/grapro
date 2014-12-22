#ifndef FRAMEWORK_GLFW_H
#define FRAMEWORK_GLFW_H

#if !defined(GLFW_INCLUDE_NONE)
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

namespace framework
{

bool initGLFW();
bool terminateGLFW();

} // namespace framework

#endif // FRAMEWORK_GLFW_H
