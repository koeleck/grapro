#include <iostream>

#include "log/log.h"
#include "log/ostream_sink.h"
#include "log/relay.h"

#include "framework/vars.h"
#include "framework/glfw.h"

#include "import/import.h"

int main()
{
    logging::Relay::initialize();
    std::shared_ptr<logging::Sink> sink = std::make_shared<logging::OStreamSink>(std::cout);
    logging::Relay::get().registerSink(sink);

    framework::initGLFW();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, vars.gl_version_major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, vars.gl_version_minor);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, vars.gl_debug_context);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* main_window = glfwCreateWindow(vars.screen_width, vars.screen_height,
            vars.title.c_str(), nullptr, nullptr);
    if (!main_window)
        return 1;
    glfwMakeContextCurrent(main_window);


    auto res = import::importSceneFile("scenes/sponza/sponza.obj");

    framework::terminateGLFW();

    logging::Relay::shutdown();

    return 0;
}
