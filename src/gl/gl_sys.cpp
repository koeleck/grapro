#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <string>

#include "gl_sys.h"
#include "log/log.h"
#include "framework/vars.h"
#include "util/debugbreak.h"

//////////////////////////////////////////////////////////////////////////

namespace
{

bool initialized = false;

std::unordered_map<GLenum, std::string> tokens;

int rank_severity(GLenum sev)
{
    if (sev == GL_DEBUG_SEVERITY_NOTIFICATION)
        return 0;
    if (sev == GL_DEBUG_SEVERITY_LOW)
        return 1;
    if (sev == GL_DEBUG_SEVERITY_MEDIUM)
        return 1;
    return 3;
}

void debug_callback(const GLenum source, const GLenum type, const GLuint id,
        const GLenum severity, const GLsizei /*length*/,
        const GLchar* const message, const void* /*userParam*/)
{
    using namespace logging;

    GLenum min_severity = gl::stringToEnum(vars.gl_debug_min_severity);
    if (rank_severity(severity) < rank_severity(min_severity))
        return;

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

void populateEnumMap()
{
#define ADD_ENUM(A) tokens.emplace(A, #A)
    ADD_ENUM(GL_LINEAR);
    ADD_ENUM(GL_NEAREST);
    ADD_ENUM(GL_NEAREST_MIPMAP_NEAREST);
    ADD_ENUM(GL_LINEAR_MIPMAP_NEAREST);
    ADD_ENUM(GL_NEAREST_MIPMAP_LINEAR);
    ADD_ENUM(GL_LINEAR_MIPMAP_LINEAR);
    ADD_ENUM(GL_DEBUG_SEVERITY_NOTIFICATION);
    ADD_ENUM(GL_DEBUG_SEVERITY_LOW);
    ADD_ENUM(GL_DEBUG_SEVERITY_MEDIUM);
    ADD_ENUM(GL_DEBUG_SEVERITY_HIGH);
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
    // check for extensions
    //
    // ...
    GLint num_extensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    std::string available_extensions;
    for (GLint i = 0; i < num_extensions; ++i) {
        available_extensions += ' ';
        available_extensions += reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
    }
#define REQUIRES_EXTENSION(EXT)                                 \
    if (available_extensions.find(#EXT) == std::string::npos) { \
            LOG_ERROR(logtag::OpenGL, #EXT " not available");   \
            abort();                                            \
    }

    REQUIRES_EXTENSION(GL_ARB_bindless_texture)
    REQUIRES_EXTENSION(GL_EXT_direct_state_access)
    REQUIRES_EXTENSION(GL_ARB_shader_draw_parameters);
    REQUIRES_EXTENSION(GL_EXT_texture_filter_anisotropic);

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

    populateEnumMap();

    initialized = true;
    return true;
}

//////////////////////////////////////////////////////////////////////////

std::string enumToString(const GLenum e)
{
    auto it = tokens.find(e);
    if (it == tokens.end()) {
        LOG_ERROR("Didn't find GLenum: ", e);
        return "FUCK";
    }
    return it->second;
}

//////////////////////////////////////////////////////////////////////////

GLenum stringToEnum(const std::string& s)
{
    auto it = std::find_if(tokens.begin(), tokens.end(), [&s]
            (std::unordered_map<GLenum, std::string>::const_reference el) -> bool
            {
                return el.second == s;
            });
    if (it == tokens.end()) {
        LOG_ERROR("Can't convert string to GLenum: ", s);
        return 0;
    }
    return it->first;
}

//////////////////////////////////////////////////////////////////////////

} // namespace gl
