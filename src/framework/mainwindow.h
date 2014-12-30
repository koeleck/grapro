#ifndef FRAMEWORK_MAINWINDOW_H
#define FRAMEWORK_MAINWINDOW_H

#include <glm/vec2.hpp>
#include "window.h"
#include "util/bitfield.h"

namespace framework
{

enum class KeyState : unsigned char
{
    NONE    = 0x00,
    PRESS   = 0x01,
    REPEAT  = 0x02,
    RELEASE = 0x04,
    SHIFT   = 0x10,
    CONTROL = 0x20,
    ALT     = 0x40,
    SUPER   = 0x80
};



class MainWindow
  : public Window
{
public:
    MainWindow(GLFWwindow* window);
    virtual ~MainWindow() override;

    void resetInput();

    virtual void update(double delta_t);
    virtual void render();

protected:
    virtual void key_event(int, int, int, int) override;
    virtual void mouse_position(double, double) override;
    virtual void mouse_button(int, int, int) override;
    virtual void mouse_enter(bool entered) override;
    virtual void mouse_scroll(double xoffset, double yoffset) override;
    virtual void window_size(int width, int height) override;

    util::bitfield<KeyState>    m_keys[350];
    glm::vec2                   m_cursor_diff;
    glm::vec2                   m_cursor_pos;
    glm::vec2                   m_mouse_scroll;
    util::bitfield<KeyState>    m_buttons[8];

private:

};

} // namespace framework

#endif // FRAMEWORK_MAINWINDOW_H

