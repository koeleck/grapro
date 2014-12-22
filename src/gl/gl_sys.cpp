#include <sstream>

#include "gl_sys.h"
#include "log/log.h"
#include "framework/vars.h"
#include "util/debugbreak.h"

//////////////////////////////////////////////////////////////////////////

namespace
{

bool initialized = false;

void debug_callback(const GLenum source, const GLenum type, const GLuint id,
        const GLenum severity, const GLsizei /*length*/,
        const GLchar* const message, const void* /*userParam*/)
{
    using namespace logging;

    if (vars.gl_debug_break) {
        debug_break();
    }

    Severity sev = Severity::Info;
    try {
        std::ostringstream msg;

        switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            sev = Severity::Error;
            //msg << "Error";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            sev = Severity::Warning;
            msg << "(Deprecated) ";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            sev = Severity::Warning;
            msg << "(Undefined) ";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            msg << "(Portability) ";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            msg << "(Performance) ";
            break;
        case GL_DEBUG_TYPE_MARKER:
            msg << "(Marker) ";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            msg << "(Push_Group) ";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            msg << "(Pop_Group) ";
            break;
        case GL_DEBUG_TYPE_OTHER:
        default:
            msg << "(Other) ";
            break;
        }

        msg << message << " [src=";
        switch (source) {
        case GL_DEBUG_SOURCE_API:
            msg << "API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            msg << "WindowSystem";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            msg << "ShaderCompiler";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            msg << "ThirdParty";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            msg << "Application";
            break;
        case GL_DEBUG_SOURCE_OTHER:
        default:
            msg << "Other";
            break;
        }

        msg << ", severity=";
        switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            msg << "high";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            msg << "medium";
            break;
        case GL_DEBUG_SEVERITY_LOW:
            msg << "low";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            msg << "notification";
            break;
        default:
            msg << "other";
            break;
        }

        msg << ", id=" << id << "]";

        log(sev, __FILE__, __LINE__, logtag::OpenGL, msg.str());

    } catch (...) {
    }
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////

namespace gl
{

//////////////////////////////////////////////////////////////////////////

bool initGL()
{
    if (initialized)
        return true;

    glewExperimental = true;
    const auto err = glewInit();
    if (err != GLEW_OK) {
        LOG_ERROR(logtag::OpenGL, "Failed to initialize GLEW: ",
            glewGetErrorString(err));
        abort();
    }

    //
    // check
    //
    // ...

    if (vars.gl_debug_context) {
        if (!glewIsSupported("GL_ARB_debug_output")) {
            LOG_WARNING(logtag::OpenGL, "GL_ARB_debug_output not available!");
        } else {
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(debug_callback, nullptr);
        }
    }

    {
        std::ostringstream msg;

        msg << "Version: " << glGetString(GL_VERSION);

        GLint flags = 0;
        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &flags);
        if (flags & GL_CONTEXT_CORE_PROFILE_BIT)
            msg << " core profile";
        if (flags & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
            msg << " compatibility profile";

        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT)
            msg << " forward compatible";
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
            msg << " debug context";

        msg << "\nGLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION);
        msg << "\nVendor: " << glGetString(GL_VENDOR);

        //int num_exts;
        //glGetIntegerv(GL_NUM_EXTENSIONS, &num_exts);
        //msg << "\nExtensions: ";
        //for (int n = 0; n < num_exts; ++n) {
        //    msg << glGetStringi(GL_EXTENSIONS, n);
        //    if (n + 1 < num_exts)
        //        msg << ", ";
        //}

        LOG_INFO(logtag::OpenGL, msg.str());
    }

    initialized = true;
    return true;
}

//////////////////////////////////////////////////////////////////////////

} // namespace gl
