#ifndef FRAMEWORK_MAINWINDOW_H
#define FRAMEWORK_MAINWINDOW_H

#include <chrono>
#include <glm/vec2.hpp>

#include "gl/gl_objects.h"
#include "window.h"
#include "util/bitfield.h"

namespace framework
{

enum class KeyState : unsigned char
{
    NONE    = 0x00,
    PRESS   = 0x01,
    REPEAT  = 0x02,
    DOWN    = 0x03,
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

    void update();
    void render();

    int getWidth() const;
    int getHeight() const;

protected:
    virtual void handle_keyboard(double delta_t);
    virtual void handle_mouse(double delta_t);
    virtual void update_gui(double delta_t);
    virtual void update_stuff(double delta_t);
    virtual void resize(int width, int height);
    virtual void render_scene();

    util::bitfield<KeyState> getKey(int key) const;
    util::bitfield<KeyState> getButton(int button) const;
    const glm::vec2& getCursorPos() const;
    const glm::vec2& getCursorDiff() const;
    const glm::vec2& getMouseScroll() const;

private:
    using clock = std::chrono::high_resolution_clock;

    void initGUI();
    virtual void key_event(int, int, int, int) override;
    virtual void mouse_position(double, double) override;
    virtual void mouse_button(int, int, int) override;
    virtual void mouse_enter(bool entered) override;
    virtual void mouse_scroll(double xoffset, double yoffset) override;
    virtual void window_size(int width, int height) override;
    virtual void char_input(unsigned int codepoint) override;

    void resetInput();

    util::bitfield<KeyState>    m_keys[350];
    glm::vec2                   m_cursor_diff;
    glm::vec2                   m_cursor_pos;
    glm::vec2                   m_mouse_scroll;
    util::bitfield<KeyState>    m_buttons[8];


    int                         m_width;
    int                         m_height;
    gl::VertexArray             m_gui_vao;
    gl::Buffer                  m_gui_vertex_buffer;
    gl::Texture                 m_gui_font_texture;
    clock::time_point           m_lastupdate;
};

} // namespace framework

#endif // FRAMEWORK_MAINWINDOW_H

