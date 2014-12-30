#include <cassert>
#include <cstdlib>

#include <GLFW/glfw3.h>

#include "window.h"
#include "glfw.h"
#include "gl/gl_sys.h"
#include "log/log.h"
#include "vars.h"

//////////////////////////////////////////////////////////////////////////

namespace
{

framework::Window* getWindowPtr(GLFWwindow* const window) noexcept
{
    assert(window != nullptr);
    void* const ptr = glfwGetWindowUserPointer(window);
    assert(ptr != nullptr);
    return static_cast<framework::Window*>(ptr);
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////

namespace framework
{

//////////////////////////////////////////////////////////////////////////

Window::Window(GLFWwindow* window)
  : m_window{window}
{

#ifndef NDEBUG
    GLFWwindow* last_context = glfwGetCurrentContext();
    if (nullptr != last_context && last_context != window) {
        LOG_WARNING(logtag::System, "Current OpenGL context "
                "replaced in glfw::Window constructor");
    }
#endif
    glfwMakeContextCurrent(m_window);
    gl::initGL();

    glfwSetWindowUserPointer(m_window, this);

    // set callbacks
    glfwSetWindowPosCallback(m_window, cb_winpos);
    glfwSetWindowSizeCallback(m_window, cb_winsize);
    glfwSetWindowCloseCallback(m_window, cb_winclose);
    glfwSetWindowFocusCallback(m_window, cb_winfocus);
    glfwSetCursorPosCallback(m_window, cb_mousepos);
    glfwSetMouseButtonCallback(m_window, cb_mousebutton);
    glfwSetCursorEnterCallback(m_window, cb_mouseenter);
    glfwSetScrollCallback(m_window, cb_mousescroll);
    glfwSetKeyCallback(m_window, cb_keyevent);
    glfwSetCharCallback(m_window, cb_charinput);
}

//////////////////////////////////////////////////////////////////////////

Window::~Window()
{
    if (m_window != nullptr) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
}

//////////////////////////////////////////////////////////////////////////

Window::Window(Window&& other) noexcept
  : m_window{other.m_window}
{
    glfwSetWindowUserPointer(m_window, this);
    other.m_window = nullptr;
}

//////////////////////////////////////////////////////////////////////////

Window& Window::operator=(Window&& other) & noexcept
{
    swap(other);
    return *this;
}

//////////////////////////////////////////////////////////////////////////

void Window::swap(Window& other) noexcept
{
    const auto tmp = m_window;
    m_window = other.m_window;
    other.m_window = tmp;
    glfwSetWindowUserPointer(this->m_window, this);
    glfwSetWindowUserPointer(other.m_window, &other);
}

//////////////////////////////////////////////////////////////////////////

void Window::window_position(const int /*xpos*/, const int /*ypos*/)
{
}

//////////////////////////////////////////////////////////////////////////

void Window::window_size(const int width, const int height)
{
}

//////////////////////////////////////////////////////////////////////////

void Window::window_close()
{
}

//////////////////////////////////////////////////////////////////////////

void Window::window_focus(const bool /*focus*/)
{
}


//////////////////////////////////////////////////////////////////////////

void Window::mouse_position(const double /*xpos*/, const double /*ypos*/)
{
}

//////////////////////////////////////////////////////////////////////////

void Window::mouse_button(const int /*button*/, const int /*action*/,
        const int /*mods*/)
{
}

//////////////////////////////////////////////////////////////////////////

void Window::mouse_enter(const bool /*entered*/)
{
}

//////////////////////////////////////////////////////////////////////////

void Window::mouse_scroll(const double /*xoffset*/, const double /*yoffset*/)
{
}

//////////////////////////////////////////////////////////////////////////

void Window::key_event(const int /*key*/, const int /*scancode*/,
        const int /*action*/, const int /*mods*/)
{
}

//////////////////////////////////////////////////////////////////////////

void Window::char_input(const unsigned int /* codepoint */)
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/*************************************************************************
 *
 * The real callback functions
 *
 ************************************************************************/

//////////////////////////////////////////////////////////////////////////

void Window::cb_winpos(GLFWwindow* const window, const int xpos,
        const int ypos) noexcept
{
    try {
        getWindowPtr(window)->window_position(xpos, ypos);
    } catch (...) {
        // TODO
    }
}

//////////////////////////////////////////////////////////////////////////

void Window::cb_winsize(GLFWwindow* const window, const int width,
        const int height) noexcept
{
    try {
        getWindowPtr(window)->window_size(width, height);
    } catch (...) {
        // TODO
    }
}

//////////////////////////////////////////////////////////////////////////

void Window::cb_winclose(GLFWwindow* const window) noexcept
{
    try {
        getWindowPtr(window)->window_close();
    } catch (...) {
        // TODO
    }
}

//////////////////////////////////////////////////////////////////////////

void Window::cb_winfocus(GLFWwindow* const window, const int focus) noexcept
{
    try {
        getWindowPtr(window)->window_focus(focus == GL_TRUE);
    } catch (...) {
        // TODO
    }
}

//////////////////////////////////////////////////////////////////////////

void Window::cb_mousepos(GLFWwindow* const window, const double xpos,
        const double ypos) noexcept
{
    try {
        getWindowPtr(window)->mouse_position(xpos, ypos);
    } catch (...) {
        // TODO
    }
}

//////////////////////////////////////////////////////////////////////////

void Window::cb_mousebutton(GLFWwindow* const window, const int button,
        const int action, const int mods) noexcept
{
    try {
        getWindowPtr(window)->mouse_button(button, action, mods);
    } catch (...) {
        // TODO
    }
}

//////////////////////////////////////////////////////////////////////////

void Window::cb_mouseenter(GLFWwindow* const window, const int entered)
{
    try {
        getWindowPtr(window)->mouse_enter(entered == GL_TRUE);
    } catch (...) {
        // TODO
    }
}

//////////////////////////////////////////////////////////////////////////

void Window::cb_mousescroll(GLFWwindow* const window, const double xoffset,
        const double yoffset) noexcept
{
    try {
        getWindowPtr(window)->mouse_scroll(xoffset, yoffset);
    } catch (...) {
        // TODO
    }
}

//////////////////////////////////////////////////////////////////////////

void Window::cb_keyevent(GLFWwindow* const window, const int key,
        const int scancode, const int action, const int mods) noexcept
{
    try {
        getWindowPtr(window)->key_event(key, scancode, action, mods);
    } catch (...) {
        // TODO
    }
}

//////////////////////////////////////////////////////////////////////////

void Window::cb_charinput(GLFWwindow* const window,
        const unsigned int codepoint) noexcept
{
    try {
        getWindowPtr(window)->char_input(codepoint);
    } catch (...) {
        // TODO
    }
}

//////////////////////////////////////////////////////////////////////////

} // namespace framework
