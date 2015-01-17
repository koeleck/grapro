#include <GLFW/glfw3.h>

#include "grapro.h"

#include "gl/opengl.h"

#include "framework/imgui.h"
#include "framework/vars.h"

#include "core/instance_manager.h"
#include "core/camera_manager.h"
#include "core/shader_manager.h"

#include "log/log.h"

#include "rendererimpl_bm.h"
#include "rendererimpl_pk.h"

/****************************************************************************/

GraPro::GraPro(GLFWwindow* window)
  : framework::MainWindow{window},
    m_cam{core::res::cameras->getDefaultCam()},
    m_showgui{true},
    m_render_bboxes{false},
    m_render_octree{false},
    m_render_voxelColors{false},
    m_render_ao{false},
    m_render_indirect{false},
    m_debug_output{false},
    m_tree_levels{static_cast<int>(vars.voxel_octree_levels)},
    m_voxelColor_level{m_tree_levels},
    m_renderer{new RendererImplBM(m_timers, m_tree_levels)}
    //m_renderer{new RendererImplPK(m_timers, m_tree_levels)}
{
    const auto* instances = core::res::instances;
    m_renderer->setGeometry(instances->getInstances());

    glm::dvec3 up {0.0, 1.0, 0.0};
    m_cam->setFixedYawAxis(true, up);

    m_render_timer = m_timers.addGPUTimer("Render");
}

/****************************************************************************/

void GraPro::render_scene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_render_timer->start();
    m_renderer->render(std::make_unsigned<int>::type(m_tree_levels),
                       std::make_unsigned<int>::type(m_voxelColor_level),
                       m_render_bboxes, m_render_octree, m_render_voxelColors,
                       m_debug_output);
    m_render_timer->stop();
}

/****************************************************************************/

void GraPro::update_gui(const double delta_t)
{
    if (!m_showgui)
        return;
    if (ImGui::Begin("DEBUG", &m_showgui)) {
        ImGui::PushItemWidth(.1f * static_cast<float>(getWidth()));

        if (ImGui::Button("recompile shaders")) {
            core::res::shaders->recompile();
            LOG_INFO("shaders recompiled");
        }

        ImGui::Spacing();

        ImGui::Checkbox("bounding boxes", &m_render_bboxes);

        // octree
        if (ImGui::CollapsingHeader("Octree", nullptr, true, true)) {
            if (ImGui::Button("rebuild octree")) {
                m_renderer->markTreeInvalid();
            }
            ImGui::Checkbox("show debug output", &m_debug_output);
            ImGui::SliderInt("tree levels", &m_tree_levels, 1, 8);
            if (m_tree_levels < m_voxelColor_level) {
                m_voxelColor_level = m_tree_levels;
            }
            ImGui::Checkbox("show voxel bounding boxes", &m_render_octree);

        }

        // other
        if (ImGui::CollapsingHeader("Other", nullptr, true, true)) {
            ImGui::Checkbox("render voxel colors", &m_render_voxelColors);
            if (m_render_voxelColors) {
                ImGui::SliderInt("voxel color level", &m_voxelColor_level, 1, m_tree_levels);
                if (m_tree_levels == 1) {
                    m_voxelColor_level = 1;
                }
            }
            ImGui::Checkbox("render Indirect", &m_render_indirect);
            m_renderer->setIndirect(m_render_indirect);
        }

        // AO
        if (ImGui::CollapsingHeader("Ambient Occlusion", nullptr, true, true)) {
            ImGui::Checkbox("render AO", &m_render_ao);
            if(m_render_ao)
            {
                ImGui::SliderInt("#cones", &m_ao_num_cones, 1, 4);
                ImGui::SliderInt("max samples", &m_ao_max_samples, 1, 10);
                ImGui::SliderInt("sample interval", &m_ao_sample_interval, 1, 100);
            }
            m_renderer->setAO(m_render_ao, m_ao_num_cones, m_ao_max_samples, m_ao_sample_interval);
        }

        // Timers: Just create your timer via m_timers and they will
        // appear here
        if (ImGui::CollapsingHeader("Time", nullptr, true, true)) {
            ImGui::Columns(2, "data", true);
            ImGui::Text("Timer");
            ImGui::NextColumn();
            ImGui::Text("Milliseconds");
            ImGui::NextColumn();
            ImGui::Separator();

            auto msecTotal = 0;
            for (const auto& t : m_timers.getTimers()) {

                if (t.first == "Render") {
                    ImGui::Text("Total");
                    ImGui::NextColumn();
                    ImGui::Text("%d ms", msecTotal);
                    ImGui::NextColumn();
                }

                ImGui::Text(t.first.c_str());
                ImGui::NextColumn();
                auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(
                        t.second->time()).count();
                ImGui::Text("%d ms", static_cast<int>(msec));
                ImGui::NextColumn();

                if (t.first != "Render") {
                    msecTotal += static_cast<int>(msec);
                }
            }
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
        movement_scale *= 20.0;
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
    if (getKey(GLFW_KEY_ESCAPE) & framework::KeyState::RELEASE)
        glfwSetWindowShouldClose(*this, GL_TRUE);
}

/****************************************************************************/

void GraPro::handle_mouse(const double delta_t)
{
    const auto rotation_scale = 0.005;
    if (glfwGetMouseButton(*this, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
        m_cam->yaw(getCursorDiff().x * rotation_scale);
        m_cam->pitch(getCursorDiff().y * rotation_scale);
    }
}

/****************************************************************************/

void GraPro::resize(const int width, const int height)
{
    auto ratio = static_cast<double>(width) / static_cast<double>(height);
    // TODO check
    static_cast<core::PerspectiveCamera*>(m_cam)->setAspectRatio(ratio);
}

/****************************************************************************/
