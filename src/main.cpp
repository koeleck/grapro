#include <iostream>
#include <boost/program_options.hpp>

#include "log/log.h"
#include "log/ostream_sink.h"
#include "log/relay.h"

#include "framework/vars.h"
#include "framework/config_file.h"
#include "framework/glfw.h"

#include "import/import.h"
#include "gl/gl_sys.h"

#include "core/loader.h"
#include "core/managers.h"

#include "grapro.h"

namespace
{
bool parseCommandLine(int argc, const char** argv);
} // anonymous namespace

int main(int argc, const char** argv)
{
    logging::Relay::initialize();
    std::shared_ptr<logging::Sink> sink = std::make_shared<logging::OStreamSink>(std::cerr);
    logging::Relay::get().registerSink(sink);

    if (!parseCommandLine(argc, argv)) {
        return 0;
    }

    framework::initGLFW();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, vars.gl_version_major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, vars.gl_version_minor);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, vars.gl_debug_context ? GL_TRUE : GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* main_window = glfwCreateWindow(vars.screen_width, vars.screen_height,
            vars.title.c_str(), nullptr, nullptr);
    if (!main_window)
        return 1;
    glfwMakeContextCurrent(main_window);
    gl::initGL();

    try {
        // INIT CORE
        core::initializeManagers();
        core::loadScenefiles(vars.scene_files);

        GraPro* win = new GraPro(main_window);

        while (!glfwWindowShouldClose(*win)) {

            win->update();
            win->render();

            glfwSwapBuffers(*win);
            glfwPollEvents();
        }

        // TERMINATE
        core::terminateManagers();

        delete win;
    } catch (...) {
        LOG_ERROR("Error"); // TODO
    }


    framework::terminateGLFW();

    logging::Relay::shutdown();

    return 0;
}

namespace {

//////////////////////////////////////////////////////////////////////////

bool parseCommandLine(const int argc, const char** argv)
{
    using namespace boost::program_options;
    bool result = true;

    try {
        bool showHelp = false;

        options_description desc("Accepted options");
        desc.add_options()
            ("config,c", value<std::string>(), "config file")
            ("help,h", "print help")
        ;

        variables_map vm;
        auto parsed = command_line_parser(argc, argv).options(desc).allow_unregistered().run();
        store(parsed, vm);
        //std::vector<std::string> additionalParameters = collect_unrecognized(parsed.options,
        //        include_positional);
        notify(vm);

        if (vm.count("help") > 0 || showHelp) {
            std::cout << "./hesp <options> [config file]" << std::endl;
            std::cout << desc;
            std::exit(0);
        }

        // TODO move this out of the function
        if (vm.count("config")) {
            framework::ConfigFile cfg(vm["config"].as<std::string>());
            vars.load(cfg);
        }

    } catch (...) {
        result = false;
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////

} // anonymous namespace
