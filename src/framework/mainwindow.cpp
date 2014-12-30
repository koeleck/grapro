#include <cstring>
#include "mainwindow.h"

#include "glfw.h"
#include "vars.h"
#include "gl/opengl.h"

namespace framework
{

/****************************************************************************/

MainWindow::MainWindow(GLFWwindow* window)
  : Window(window)
{
    resetInput();
}

/****************************************************************************/

MainWindow::~MainWindow()
{
}

/****************************************************************************/

void MainWindow::resetInput()
{
    double xpos, ypos;
    glfwGetCursorPos(*this, &xpos, &ypos);
    m_cursor_pos = glm::vec2(xpos, ypos);
    m_cursor_diff = glm::vec2(.0f);
    m_mouse_scroll = glm::vec2(.0f);

    std::memset(m_keys, 0, sizeof(m_keys));
    std::memset(m_buttons, 0, sizeof(m_buttons));
}

/****************************************************************************/

void MainWindow::update(const double delta_t)
{
}

/****************************************************************************/

void MainWindow::render()
{
}

/****************************************************************************/

void MainWindow::window_size(const int width, const int height)
{
    glViewport(0, 0, width, height);
    vars.screen_width = width;
    vars.screen_height = height;
}

/****************************************************************************/

void MainWindow::key_event(const int key, int, const int action, const int mods)
{
    auto& state = m_keys[key];
    switch (action) {
    case GLFW_PRESS:
        state = KeyState::PRESS;
        break;
    case GLFW_REPEAT:
        state = KeyState::REPEAT;
        break;
    case GLFW_RELEASE:
        state = KeyState::RELEASE;
        break;
    default:
        abort();
    }
    if (mods & GLFW_MOD_ALT)
        state |= KeyState::ALT;
    if (mods & GLFW_MOD_SHIFT)
        state |= KeyState::SHIFT;
    if (mods & GLFW_MOD_CONTROL)
        state |= KeyState::CONTROL;

}

/****************************************************************************/

void MainWindow::mouse_position(double xpos, double ypos)
{
    glm::vec2 cursor(xpos, ypos);
    m_cursor_diff += cursor - m_cursor_pos;
    m_cursor_pos = cursor;
}

/****************************************************************************/

void MainWindow::mouse_button(const int button, const int action, int const mods)
{
    auto& state = m_buttons[button];
    state = (action == GLFW_PRESS) ? KeyState::PRESS : KeyState::RELEASE;

    if (mods & GLFW_MOD_ALT)
        state |= KeyState::ALT;
    if (mods & GLFW_MOD_SHIFT)
        state |= KeyState::SHIFT;
    if (mods & GLFW_MOD_CONTROL)
        state |= KeyState::CONTROL;
}

/****************************************************************************/

void MainWindow::mouse_enter(const bool entered)
{
    if (entered) {
        double xpos, ypos;
        glfwGetCursorPos(*this, &xpos, &ypos);
        m_cursor_pos = glm::vec2(xpos, ypos);
        m_cursor_diff = glm::vec2(.0f);
    }
}

/****************************************************************************/

void MainWindow::mouse_scroll(const double xoffset, const double yoffset)
{
    m_mouse_scroll += glm::vec2(xoffset, yoffset);
}

/****************************************************************************/

} // namespace framework
