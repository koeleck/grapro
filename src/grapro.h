#ifndef GRAPRO_H
#define GRAPRO_H

#include "framework/mainwindow.h"
#include "renderer.h"
#include "core/camera.h"
#include "core/timer_array.h"

class GraPro
  : public framework::MainWindow
{
public:
    GraPro(GLFWwindow* window);

    virtual void render_scene() override;
    virtual void handle_keyboard(double delta_t) override;
    virtual void handle_mouse(double delta_t) override;
    virtual void update_gui(double delta_t) override;
    virtual void resize(int width, int height) override;

private:
    Renderer            m_renderer;
    core::Camera*       m_cam;
    bool                m_showgui;
    core::TimerArray    m_timers;
    core::GPUTimer*     m_render_timer;
};

#endif // GRAPRO_H

