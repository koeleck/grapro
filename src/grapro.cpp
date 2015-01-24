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

#include "rendererimpl_bm.h"
#include "rendererimpl_pk.h"

/****************************************************************************/

GraPro::GraPro(GLFWwindow* window)
  : framework::MainWindow{window},
    m_showgui{true},
    m_treeLevels{static_cast<int>(vars.voxel_octree_levels)},
    m_renderBBoxes{false},
    m_renderVoxelBoxes{false},
    m_renderVoxelColors{false},
    m_renderAO{false},
    m_renderIndirectDiffuse{false},
    m_renderIndirectSpecular{false},
    m_renderConeTracing{false},
    m_aoConeGridSize{10},
    m_aoConeSteps{2},
    m_aoWeight{1},
    m_diffuseConeGridSize{10},
    m_diffuseConeSteps{2},
    m_specularConeSteps{4},
    m_debugOutput{false},
    m_cam{core::res::cameras->getDefaultCam()},
    m_renderer{new RendererImplBM(m_timers, m_treeLevels)}
{
    m_options.treeLevels = m_treeLevels;
    m_options.renderBBoxes = m_renderBBoxes;
    m_options.renderVoxelBoxes = m_renderVoxelBoxes;
    m_options.renderVoxelColors = m_renderVoxelColors;
    m_options.renderAO = m_renderAO;
    m_options.renderIndirectDiffuse = m_renderIndirectDiffuse;
    m_options.renderIndirectSpecular = m_renderIndirectSpecular;
    m_options.renderConeTracing = m_renderConeTracing;
    m_options.aoConeGridSize = m_aoConeGridSize;
    m_options.aoConeSteps = m_aoConeSteps;
    m_options.aoWeight = m_aoWeight;
    m_options.diffuseConeGridSize = m_diffuseConeGridSize;
    m_options.diffuseConeSteps = m_diffuseConeSteps;
    m_options.specularConeSteps = m_specularConeSteps;
    m_options.debugOutput = m_debugOutput;

    const auto* instances = core::res::instances;
    m_renderer->setGeometry(instances->getInstances());

    glm::dvec3 up {0.0, 1.0, 0.0};
    m_cam->setFixedYawAxis(true, up);

    m_renderTimer = m_timers.addGPUTimer("Render");

    // fix lights
    const auto& bbox = m_renderer.get()->getSceneBBox();
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

    m_renderTimer->start();
    m_renderer->render(m_options);
    m_renderTimer->stop();
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

        ImGui::Checkbox("bounding boxes", &m_renderBBoxes);

        // octree
        if (ImGui::CollapsingHeader("Octree", nullptr, true, true)) {
            ImGui::Checkbox("show debug output", &m_debugOutput);
            ImGui::SliderInt("tree levels", &m_treeLevels, 1, 9);
            ImGui::Checkbox("show voxel bounding boxes", &m_renderVoxelBoxes);
        }

        // conetracing
        if (ImGui::CollapsingHeader("Conetracing", nullptr, true, true)) {
            ImGui::SliderInt("AO cone grid size", &m_aoConeGridSize, 2, 32);
            ImGui::SliderInt("AO cone steps", &m_aoConeSteps, 1, 10);
            ImGui::SliderInt("AO weight", &m_aoWeight, 0, 3);
            ImGui::SliderInt("diffuse cone grid size", &m_diffuseConeGridSize, 2, 32);
            ImGui::SliderInt("diffuse cone steps", &m_diffuseConeSteps, 1, 10);
            ImGui::SliderInt("specular cone steps", &m_specularConeSteps, 1, 10);

            ImGui::Checkbox("render ConeTracing", &m_renderConeTracing);
            ImGui::Checkbox("render Voxel Colors", &m_renderVoxelColors);
            ImGui::Checkbox("render AO", &m_renderAO);
            ImGui::Checkbox("render Indirect Diffuse", &m_renderIndirectDiffuse);
            ImGui::Checkbox("render Indirect Specular", &m_renderIndirectSpecular);
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

    m_options.treeLevels = m_treeLevels;
    m_options.renderBBoxes = m_renderBBoxes;
    m_options.renderVoxelBoxes = m_renderVoxelBoxes;
    m_options.renderVoxelColors = m_renderVoxelColors;
    m_options.renderAO = m_renderAO;
    m_options.renderIndirectDiffuse = m_renderIndirectDiffuse;
    m_options.renderIndirectSpecular = m_renderIndirectSpecular;
    m_options.renderConeTracing = m_renderConeTracing;
    m_options.aoConeGridSize = m_aoConeGridSize;
    m_options.aoConeSteps = m_aoConeSteps;
    m_options.aoWeight = m_aoWeight;
    m_options.diffuseConeGridSize = m_diffuseConeGridSize;
    m_options.diffuseConeSteps = m_diffuseConeSteps;
    m_options.specularConeSteps = m_specularConeSteps;
    m_options.debugOutput = m_debugOutput;
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
