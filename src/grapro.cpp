#include "grapro.h"

#include "gl/opengl.h"
#include "framework/imgui.h"

/****************************************************************************/

void GraPro::render_scene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/****************************************************************************/

void GraPro::update_gui(const double delta_t)
{
    static bool open = true;
    ImGui::ShowTestWindow(&open);
}

/****************************************************************************/

