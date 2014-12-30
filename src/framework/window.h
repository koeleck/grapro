#ifndef FRAMEWORK_WINDOW_H
#define FRAMEWORK_WINDOW_H

#include <string>
struct GLFWwindow;

namespace framework
{

class Window
{
public:
    // no copying
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    explicit Window(GLFWwindow* window);

    virtual ~Window();

    Window(Window&& other) noexcept;

    Window& operator=(Window&& other) & noexcept;

    void swap(Window& other) noexcept;

    operator GLFWwindow*() const noexcept {
        return m_window;
    }

    GLFWwindow* get() const noexcept {
        return m_window;
    }

protected:
    virtual void window_position(int xpos, int ypos);
    virtual void window_size(int width, int height);
    virtual void window_close();
    virtual void window_focus(bool focus);

    virtual void mouse_position(double xpos, double ypos);
    virtual void mouse_button(int button, int action, int mods);
    virtual void mouse_enter(bool entered);
    virtual void mouse_scroll(double xoffset, double yoffset);
    virtual void key_event(int key, int scancode, int action, int mods);
    virtual void char_input(unsigned int codepoint);


private:
    static void cb_winpos(GLFWwindow*, int, int) noexcept;
    static void cb_winsize(GLFWwindow*, int, int) noexcept;
    static void cb_winclose(GLFWwindow*) noexcept;
    static void cb_winfocus(GLFWwindow*, int) noexcept;
    static void cb_mousepos(GLFWwindow*, double, double) noexcept;
    static void cb_mousebutton(GLFWwindow*, int, int, int) noexcept;
    static void cb_mouseenter(GLFWwindow*, int);
    static void cb_mousescroll(GLFWwindow*, double, double) noexcept;
    static void cb_keyevent(GLFWwindow*, int, int, int, int) noexcept;
    static void cb_charinput(GLFWwindow*, unsigned int) noexcept;

    GLFWwindow*         m_window;
};

} // namespace framework

#endif // FRAMEWORK_WINDOW_H

