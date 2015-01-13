#ifndef GRAPRO_H
#define GRAPRO_H

#include "framework/mainwindow.h"

#include "core/camera.h"
#include "core/timer_array.h"

#include "rendererimpl_bm.h"
#include "rendererimpl_pk.h"
#include "rendererinterface.h"

#include <memory>

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
    core::Camera*                       m_cam;
    bool                                m_showgui;
    core::TimerArray                    m_timers;
    core::GPUTimer*                     m_render_timer;
    bool                                m_render_bboxes;
    bool                                m_render_octree;
    bool                                m_debug_output;
    int                                 m_tree_levels;

    std::unique_ptr<RendererInterface>  m_renderer;

};

#endif // GRAPRO_H

