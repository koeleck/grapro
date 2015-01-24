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

    // GUI
    bool                                m_showgui;

    int                                 m_treeLevels;

    bool                                m_renderBBoxes;
    bool                                m_renderVoxelBoxes;
    bool                                m_renderVoxelColors;
    bool                                m_renderAO;
    bool                                m_renderIndirectDiffuse;
    bool                                m_renderIndirectSpecular;
    bool                                m_renderConeTracing;

    int                                 m_aoConeGridSize;
    int                                 m_aoConeSteps;
    int                                 m_aoWeight;
    int                                 m_diffuseConeGridSize;
    int                                 m_diffuseConeSteps;
    int                                 m_specularConeSteps;

    bool                                m_debugOutput;

    Options                             m_options;

    // other
    core::Camera*                       m_cam;
    core::TimerArray                    m_timers;
    core::GPUTimer*                     m_renderTimer;
    std::unique_ptr<RendererInterface>  m_renderer;

};

#endif // GRAPRO_H

