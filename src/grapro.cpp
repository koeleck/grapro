#include <GLFW/glfw3.h>

#include "grapro.h"

#include "gl/opengl.h"
#include "framework/imgui.h"
#include "framework/vars.h"
#include "core/instance_manager.h"
#include "core/camera_manager.h"
#include "core/shader_manager.h"
#include "core/light_manager.h"
#include "log/log.h"

/****************************************************************************/

GraPro::GraPro(GLFWwindow* window)
  : framework::MainWindow(window),
    m_cam(core::res::cameras->getDefaultCam()),
    m_showgui{true},
    m_renderer{getWidth(), getHeight(), m_timers}
{

    m_options.treeLevels = static_cast<int>(vars.voxel_octree_levels);
    m_options.debugLevel = static_cast<int>(vars.voxel_octree_levels);
    m_options.renderBBoxes = false;
    m_options.renderVoxelBoxes = false;
    m_options.renderVoxelColors = false;
    m_options.renderAO = false;
    m_options.renderIndirectDiffuse = false;
    m_options.renderIndirectSpecular = false;
    m_options.renderConeTracing = false;
    m_options.aoConeGridSize = 10;
    m_options.aoConeSteps = 2;
    m_options.aoWeight = 1;
    m_options.diffuseConeGridSize = 10;
    m_options.diffuseConeSteps = 2;
    m_options.specularConeSteps = 4;
    m_options.debugOutput = false;
    m_options.debugGBuffer = false;

    const auto* instances = core::res::instances;
    m_renderer.setGeometry(instances->getInstances());

    glm::dvec3 up(0.0, 1.0, 0.0);
    m_cam->setFixedYawAxis(true, up);

    m_render_timer = m_timers.addGPUTimer("Render");

    // fix lights
    const auto& bbox = m_renderer.getSceneBBox();
    const auto maxExtend = bbox.maxExtend();
    const auto maxDist = bbox.pmax[maxExtend] - bbox.pmin[maxExtend];
    for (auto& light : core::res::lights->getLights()) {
        light->setMaxDistance(2500.f);
        if (light->getType() != core::LightType::DIRECTIONAL ||
            light->isShadowcasting() == false)
        {
            continue;
        }
        auto pos = light->getPosition();
        pos.y = bbox.pmax.y;
        light->setPosition(pos);
        auto* dlight = static_cast<core::DirectionalLight*>(light.get());
        dlight->setSize(maxDist);
    }

    // done. print gpu mem usage
    gl::printInfo();
}

/****************************************************************************/

void GraPro::render_scene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_render_timer->start();
    m_renderer.render(m_options);
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

        if (ImGui::Button("toggle shadows")) {
            m_renderer.toggleShadows();
            LOG_INFO("shadows toggled");
        }

        if (ImGui::Button("show gbuffer")) {
            m_options.debugGBuffer = !m_options.debugGBuffer;
            LOG_INFO("displaying gbuffer");
        }

        ImGui::Spacing();

        ImGui::Checkbox("bounding boxes", &m_options.renderBBoxes);
        ImGui::Checkbox("show voxel boxes", &m_options.renderVoxelBoxes);
        ImGui::Checkbox("show voxel colors", &m_options.renderVoxelColors);

        static const int max_levels = static_cast<int>(vars.voxel_octree_levels);
        ImGui::SliderInt("Tree levels", &m_options.treeLevels, 1, max_levels);
        if (vars.voxel_octree_levels != static_cast<unsigned int>(m_options.treeLevels)) {
            vars.voxel_octree_levels = static_cast<unsigned int>(m_options.treeLevels);
            m_options.debugLevel = std::min(m_options.debugLevel, m_options.treeLevels - 1);
            m_renderer.markTreeInvalid();
        }
        if (m_options.renderVoxelBoxes)
            ImGui::SliderInt("Debug tree level", &m_options.debugLevel, 0, m_options.treeLevels - 1);

        // Timers: Just create your timer via m_timers and they will
        // appear here
        if (ImGui::CollapsingHeader("Time", nullptr, false, true)) {
            ImGui::Columns(2, "data", true);
            ImGui::Text("Timer");
            ImGui::NextColumn();
            ImGui::Text("Milliseconds");
            ImGui::NextColumn();
            ImGui::Separator();

            for (const auto& t : m_timers.getTimers()) {
                ImGui::Text(t.first.c_str());
                ImGui::NextColumn();
                auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(
                        t.second->time()).count();
                ImGui::Text("%d ms", static_cast<int>(msec));
                ImGui::NextColumn();
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
    if (glfwGetKey(*this, GLFW_KEY_C) == GLFW_PRESS)
        m_cam->lookAt(glm::dvec3(0.));

    // misc:
    if (getKey(GLFW_KEY_T) & framework::KeyState::PRESS)
        m_showgui = !m_showgui;
    if (getKey(GLFW_KEY_ESCAPE) & framework::KeyState::RELEASE)
        glfwSetWindowShouldClose(*this, GL_TRUE);
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
    m_renderer.resize(width, height);
}

/****************************************************************************/
