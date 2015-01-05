#include <GLFW/glfw3.h>

#include "grapro.h"

#include "gl/opengl.h"
#include "framework/imgui.h"
#include "core/instance_manager.h"
#include "core/camera_manager.h"
#include "core/shader_manager.h"
#include "log/log.h"

/****************************************************************************/

GraPro::GraPro(GLFWwindow* window)
  : framework::MainWindow(window),
    m_cam(core::res::cameras->getDefaultCam()),
    m_showgui{true}
{
    const auto* instances = core::res::instances;
    m_renderer.setGeometry(instances->getInstances());

    glm::dvec3 up(0.0, 1.0, 0.0);
    m_cam->setFixedYawAxis(true, up);

}

/****************************************************************************/

void GraPro::render_scene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_renderer.render();
}

/****************************************************************************/

void GraPro::update_gui(const double delta_t)
{
    if (!m_showgui)
        return;
    if (ImGui::Begin("DEBUG", &m_showgui)) {
        ImGui::PushItemWidth(.65f * static_cast<float>(getWidth()));

        if (ImGui::Button("recompile shaders")) {
            core::res::shaders->recompile();
            LOG_INFO("shaders recompiled");
        }
    }
    ImGui::End();
}

/****************************************************************************/

void GraPro::handle_keyboard(const double delta_t)
{
    // Camera:
    double movement_scale = 2.0;
    if (glfwGetKey(*this, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        movement_scale *= 5.0;
    if (glfwGetKey(*this, GLFW_KEY_W) == GLFW_PRESS)
        m_cam->move(movement_scale * glm::dvec3(0.0, 0.0, 1.0));
    if (glfwGetKey(*this, GLFW_KEY_S) == GLFW_PRESS)
        m_cam->move(movement_scale * glm::dvec3(0.0, 0.0, -1.0));
    if (glfwGetKey(*this, GLFW_KEY_D) == GLFW_PRESS)
        m_cam->move(movement_scale * glm::dvec3(1.0, 0.0, 0.0));
    if (glfwGetKey(*this, GLFW_KEY_A) == GLFW_PRESS)
        m_cam->move(movement_scale * glm::dvec3(-1.0, 0.0, 0.0));
    if (glfwGetKey(*this, GLFW_KEY_R) == GLFW_PRESS)
        m_cam->move(movement_scale * glm::dvec3(0.0, 1.0, 0.0));
    if (glfwGetKey(*this, GLFW_KEY_F) == GLFW_PRESS)
        m_cam->move(movement_scale * glm::dvec3(0.0, -1.0, 0.0));

    // misc:
    if (getKey(GLFW_KEY_T) & framework::KeyState::PRESS)
        m_showgui = !m_showgui;
}

/****************************************************************************/

void GraPro::handle_mouse(const double delta_t)
{
    const double rotation_scale = 0.005;
    if (glfwGetMouseButton(*this, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
        m_cam->yaw(getCursorDiff().x * rotation_scale);
        m_cam->pitch(getCursorDiff().y * rotation_scale);
    }
}

/****************************************************************************/

void GraPro::resize(const int width, const int height)
{
    double ratio = static_cast<double>(width) / height;
    // TODO check
    static_cast<core::PerspectiveCamera*>(m_cam)->setAspectRatio(ratio);
}

/****************************************************************************/
