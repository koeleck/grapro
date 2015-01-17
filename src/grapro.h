#ifndef GRAPRO_H
#define GRAPRO_H

#include <memory>

#include "framework/mainwindow.h"

#include "core/timer_array.h"

#include "rendererinterface.h"

namespace core {
    class Camera;
}

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
    bool                                m_render_voxelColors;
    bool                                m_debug_output;
    int                                 m_tree_levels;
    int                                 m_voxelColor_level;

    std::unique_ptr<RendererInterface>  m_renderer;

};

#endif // GRAPRO_H

