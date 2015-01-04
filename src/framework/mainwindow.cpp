#include <cstring>
#include <cstddef>
#include "mainwindow.h"

#include "glfw.h"
#include "vars.h"
#include "gl/opengl.h"
#include "core/shader_manager.h"
#include "imgui.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace
{

struct
{
    GLuint  fontTex;
    GLuint  vbo_handle;
    GLuint  vao_handle;
    core::Program program;
    GLint   texture_location;
    GLint   ortho_location;
    GLFWwindow* window;
} GuiData;

static size_t vbo_max_size = 20000;

void ImImpl_RenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count)
{
    if (cmd_lists_count == 0)
        return;
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    // Setup texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, GuiData.fontTex);

    const float width = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;
    const float ortho_projection[4][4] =
    {
        { 2.0f/width, 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f/-height, 0.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f, 0.0f },
        { -1.0f, 1.0f, 0.0f, 1.0f },
    };
    glUseProgram(GuiData.program);
    glUniform1i(GuiData.texture_location, 0);
    glUniformMatrix4fv(GuiData.ortho_location, 1, GL_FALSE, &ortho_projection[0][0]);

    // Grow our buffer according to what we need
    size_t total_vtx_count = 0;
    for (int n = 0; n < cmd_lists_count; n++)
        total_vtx_count += cmd_lists[n]->vtx_buffer.size();

    glBindBuffer(GL_ARRAY_BUFFER, GuiData.vbo_handle);
    size_t neededBufferSize = total_vtx_count * sizeof(ImDrawVert);
    if (neededBufferSize > vbo_max_size) {
        vbo_max_size = neededBufferSize + 5000; // Grow buffer
        glBufferData(GL_ARRAY_BUFFER, vbo_max_size, NULL, GL_STREAM_DRAW);
    }

    // Copy and convert all vertices into a single contiguous buffer
    unsigned char* buffer_data = (unsigned char*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    if (!buffer_data)
        return;
    for (int n = 0; n < cmd_lists_count; n++) {
        const ImDrawList* cmd_list = cmd_lists[n];
        std::memcpy(buffer_data, &cmd_list->vtx_buffer[0], cmd_list->vtx_buffer.size() * sizeof(ImDrawVert));
        buffer_data += cmd_list->vtx_buffer.size() * sizeof(ImDrawVert);
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(GuiData.vao_handle);
    int cmd_offset = 0;
    for (int n = 0; n < cmd_lists_count; n++) {
        const ImDrawList* cmd_list = cmd_lists[n];
        int vtx_offset = cmd_offset;
        for (const auto& pcmd : cmd_list->commands) {
            glScissor((int)pcmd.clip_rect.x,
                      (int)(height - pcmd.clip_rect.w),
                      (int)(pcmd.clip_rect.z - pcmd.clip_rect.x),
                      (int)(pcmd.clip_rect.w - pcmd.clip_rect.y));
            glDrawArrays(GL_TRIANGLES, vtx_offset, static_cast<GLsizei>(pcmd.vtx_count));
            vtx_offset += pcmd.vtx_count;
        }
        cmd_offset = vtx_offset;
    }
    // Restore modified state
    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_SCISSOR_TEST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

const char* ImImpl_GetClipboardTextFn()
{
    return glfwGetClipboardString(GuiData.window);
}

void ImImpl_SetClipboardTextFn(const char* text)
{
    glfwSetClipboardString(GuiData.window, text);
}

} // anonymous namespace

namespace framework
{

/****************************************************************************/

MainWindow::MainWindow(GLFWwindow* window)
  : Window(window),
    m_gui_hidden{false},
    m_lastupdate{clock::now()}
{
    resetInput();
    initGUI();
}

/****************************************************************************/

MainWindow::~MainWindow()
{
}

/****************************************************************************/

void MainWindow::resetInput()
{
    double xpos, ypos;
    glfwGetCursorPos(*this, &xpos, &ypos);
    m_cursor_pos = glm::vec2(xpos, ypos);
    m_cursor_diff = glm::vec2(.0f);
    m_mouse_scroll = glm::vec2(.0f);

    std::memset(m_keys, 0, sizeof(m_keys));
    std::memset(m_buttons, 0, sizeof(m_buttons));

    auto& io = ImGui::GetIO();
    io.MouseWheel = 0.0f;
    memset(io.InputCharacters, 0, sizeof(io.InputCharacters));
    io.MouseDown[0] = 0;
    io.MouseDown[1] = 0;
}

/****************************************************************************/

void MainWindow::update()
{
    auto time = clock::now();
    const double delta_t =
            static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>
            (time - m_lastupdate).count()) / 1000000.;
    m_lastupdate = time;

    if (!m_gui_hidden) {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(static_cast<float>(m_width), static_cast<float>(m_height));
        io.DeltaTime = static_cast<float>(delta_t);
        // Setup inputs
        io.MousePos = ImVec2(m_cursor_pos.x, m_cursor_pos.y);
        io.MouseDown[0] = !!(m_buttons[0] & KeyState::DOWN) ||
                glfwGetMouseButton(*this, GLFW_MOUSE_BUTTON_LEFT) != 0;
        io.MouseDown[1] = !!(m_buttons[1] & KeyState::DOWN) ||
                glfwGetMouseButton(*this, GLFW_MOUSE_BUTTON_RIGHT) != 0;
        // Start the frame
        ImGui::NewFrame();
        update_gui(delta_t);
    }

    if (ImGui::GetIO().WantCaptureMouse == false)
        handle_mouse(delta_t);
    if (ImGui::GetIO().WantCaptureKeyboard == false)
        handle_keyboard(delta_t);
    update_stuff(delta_t);

    resetInput();
}

/****************************************************************************/

void MainWindow::render()
{
    render_scene();
    if (!m_gui_hidden)
        ImGui::Render();
}

/****************************************************************************/

void MainWindow::window_size(const int new_width, const int new_height)
{
    glViewport(0, 0, new_width, new_height);
    vars.screen_width = new_width;
    vars.screen_height = new_height;
    m_width = new_width;
    m_height = new_height;
    resize(new_width, new_height);
}

/****************************************************************************/

void MainWindow::key_event(const int key, int, const int action, const int mods)
{
    auto& state = m_keys[key];
    switch (action) {
    case GLFW_PRESS:
        state = KeyState::PRESS;
        break;
    case GLFW_REPEAT:
        state = KeyState::REPEAT;
        break;
    case GLFW_RELEASE:
        state = KeyState::RELEASE;
        break;
    default:
        return;
    }
    if (mods & GLFW_MOD_ALT)
        state |= KeyState::ALT;
    if (mods & GLFW_MOD_SHIFT)
        state |= KeyState::SHIFT;
    if (mods & GLFW_MOD_CONTROL)
        state |= KeyState::CONTROL;

    ImGuiIO& io = ImGui::GetIO();
    if (action == GLFW_PRESS)
        io.KeysDown[key] = true;
    if (action == GLFW_RELEASE)
        io.KeysDown[key] = false;
    io.KeyCtrl = (mods & GLFW_MOD_CONTROL) != 0;
    io.KeyShift = (mods & GLFW_MOD_SHIFT) != 0;
}

/****************************************************************************/

void MainWindow::mouse_position(double xpos, double ypos)
{
    glm::vec2 cursor(xpos, ypos);
    m_cursor_diff += cursor - m_cursor_pos;
    m_cursor_pos = cursor;
}

/****************************************************************************/

void MainWindow::mouse_button(const int button, const int action, int const mods)
{
    auto& state = m_buttons[button];
    state = (action == GLFW_PRESS) ? KeyState::PRESS : KeyState::RELEASE;

    if (mods & GLFW_MOD_ALT)
        state |= KeyState::ALT;
    if (mods & GLFW_MOD_SHIFT)
        state |= KeyState::SHIFT;
    if (mods & GLFW_MOD_CONTROL)
        state |= KeyState::CONTROL;
}

/****************************************************************************/

void MainWindow::mouse_enter(const bool entered)
{
    if (entered) {
        double xpos, ypos;
        glfwGetCursorPos(*this, &xpos, &ypos);
        m_cursor_pos = glm::vec2(xpos, ypos);
        m_cursor_diff = glm::vec2(.0f);
    }
}

/****************************************************************************/

void MainWindow::mouse_scroll(const double xoffset, const double yoffset)
{
    m_mouse_scroll += glm::vec2(xoffset, yoffset);
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel += static_cast<float>(yoffset);
}

/****************************************************************************/

int MainWindow::getWidth() const
{
    return m_width;
}

/****************************************************************************/

int MainWindow::getHeight() const
{
    return m_height;
}

/****************************************************************************/

void MainWindow::showGUI()
{
    m_gui_hidden = false;
}

/****************************************************************************/

void MainWindow::hideGUI()
{
    m_gui_hidden = true;
}

/****************************************************************************/

void MainWindow::toggleGUI()
{
    m_gui_hidden = !m_gui_hidden;
}

/****************************************************************************/

bool MainWindow::GUIStatus() const
{
    return !m_gui_hidden;
}

/****************************************************************************/

void MainWindow::handle_keyboard(double /* delta_t */)
{
}
/****************************************************************************/

void MainWindow::handle_mouse(double /* delta_t */)
{
}

/****************************************************************************/

void MainWindow::update_gui(double /* delta_t */)
{
}

/****************************************************************************/

void MainWindow::update_stuff(double /* delta_t */)
{
}

/****************************************************************************/

util::bitfield<KeyState> MainWindow::getKey(int key) const
{
    return m_keys[key];
}

/****************************************************************************/

util::bitfield<KeyState> MainWindow::getButton(int button) const
{
    return m_buttons[button];
}

/****************************************************************************/

const glm::vec2& MainWindow::getCursorPos() const
{
    return m_cursor_pos;
}

/****************************************************************************/

const glm::vec2& MainWindow::getCursorDiff() const
{
    return m_cursor_diff;
}

/****************************************************************************/

const glm::vec2& MainWindow::getMouseScroll() const
{
    return m_mouse_scroll;
}

/****************************************************************************/

void MainWindow::resize(const int width, const int height)
{
    glViewport(0, 0, width, height);
}

/****************************************************************************/

void MainWindow::render_scene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/****************************************************************************/

void MainWindow::initGUI()
{
    core::res::shaders->registerShader("imgui_vertex_shader",
            "gui/gui.vert", GL_VERTEX_SHADER);
    core::res::shaders->registerShader("imgui_fragment_shader",
            "gui/gui.frag", GL_FRAGMENT_SHADER);
    auto prog = core::res::shaders->registerProgram("imgui_program",
            {"imgui_vertex_shader", "imgui_fragment_shader"});

    GuiData.program = prog;
    GuiData.texture_location = glGetUniformLocation(GuiData.program, "Texture");
    GuiData.ortho_location = glGetUniformLocation(GuiData.program, "ortho");
    GuiData.vbo_handle = m_gui_vertex_buffer.get();
    GuiData.vao_handle = m_gui_vao.get();
    GuiData.window = *this;
    GuiData.fontTex = m_gui_font_texture.get();

    glBindVertexArray(m_gui_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_gui_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, vbo_max_size, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
            reinterpret_cast<GLvoid*>(offsetof(ImDrawVert, pos)));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
            reinterpret_cast<GLvoid*>(offsetof(ImDrawVert, uv)));
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert),
            reinterpret_cast<GLvoid*>(offsetof(ImDrawVert, col)));
    glBindVertexArray(0);

    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = 1.0f / 60.0f; // Time elapsed since last frame, in seconds (in this sample app we'll override this every frame because our timestep is variable)
    io.PixelCenterOffset = 0.5f; // Align OpenGL texels
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB; // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;
    io.RenderDrawListsFn = ImImpl_RenderDrawLists;
    io.SetClipboardTextFn = ImImpl_SetClipboardTextFn;
    io.GetClipboardTextFn = ImImpl_GetClipboardTextFn;
    // Load font texture
    glBindTexture(GL_TEXTURE_2D, m_gui_font_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    const void* png_data;
    unsigned int png_size;
    ImGui::GetDefaultFontData(NULL, NULL, &png_data, &png_size);
    int tex_x, tex_y, tex_comp;
    void* tex_data = stbi_load_from_memory((const unsigned char*)png_data, (int)png_size, &tex_x, &tex_y, &tex_comp, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_x, tex_y, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
    stbi_image_free(tex_data);
}

/****************************************************************************/

void MainWindow::char_input(unsigned int codepoint)
{
    if (codepoint > 0 && codepoint < 0x10000)
        ImGui::GetIO().AddInputCharacter(static_cast<unsigned short>(codepoint));
}

/****************************************************************************/

} // namespace framework
