#include <algorithm>
#include <cassert>
#include <fstream>
#include <memory>
#include <cstring>
#include <boost/tokenizer.hpp>

#include "shader_manager.h"

#include "framework/vars.h"
#include "log/log.h"
#include "gl/program.h"
#include "gl/shader.h"
#include "gl/shadersource.h"
#include "gl/gl_objects.h"

#include <boost/filesystem.hpp>
using namespace boost::filesystem;

/****************************************************************************/

namespace
{

struct CacheFile
{
    GLenum      binary_format;
    int         num_shaders;
    int         num_files;
    bool        is_separable;
};

} // anonmymous namespace

/****************************************************************************/

namespace core
{

/****************************************************************************/

struct ShaderManager::ShaderInfo
{
    std::string                 filename;
    std::vector<std::string>    files;
    std::string                 defines;
    GLenum                      type;
    gl::Shader                  shader;
};


/****************************************************************************/

struct ShaderManager::ProgramInfo
{
    ProgramInfo() : program{gl::NO_GEN} {}
    std::vector<std::string>    shaders;
    std::vector<std::string>    files;
    bool                        is_separable;
    gl::Program                 program;
};

/****************************************************************************/

struct ShaderManager::PipelineInfo
{
    PipelineInfo() : pipeline{gl::NO_GEN} {}
    std::vector<std::string>    programs;
    gl::ProgramPipeline         pipeline;
};

/****************************************************************************/

ShaderManager::ShaderManager()
  : m_timestamp{std::time(nullptr)}
{
}

/****************************************************************************/

void ShaderManager::registerShader(const std::string& name, const std::string& filename,
        const GLenum shader_type, const std::string& defines)
{
    auto it = m_shader_names.find(name);
    if (it != m_shader_names.end()) {
        const auto& info = m_shaders[it->second];
        if (info.filename != filename ||
            info.defines != defines ||
            info.type != shader_type)
        {
            LOG_ERROR("Redefinition of already existing shader '", name,"'");
            abort();
        }
        return;
    }

    // check if the same shader (with different name) already exists
    std::size_t idx;
    for (idx = 0; idx < m_shaders.size(); ++idx) {
        const auto& info = m_shaders[idx];
        if (info.filename == filename &&
            info.defines == defines &&
            info.type == shader_type)
        {
            break;
        }
    }

    if (idx == m_shaders.size()) {
        m_shaders.emplace_back();

        auto& info = m_shaders.back();
        info.filename = filename;
        info.type = shader_type;
        info.defines = defines;

    }
    m_shader_names.emplace(name, idx);
}

/****************************************************************************/

void ShaderManager::registerProgram(const std::string& name, const bool is_separable,
        std::initializer_list<std::string> shaders)
{
    std::vector<std::string> shader_names(shaders);
    std::sort(shader_names.begin(), shader_names.end());

    auto it = m_program_names.find(name);
    if (it != m_program_names.end()) {
        const auto& info = m_programs[it->second];
        if (info.is_separable != is_separable ||
            info.shaders.size() != shader_names.size() ||
            !std::equal(info.shaders.cbegin(), info.shaders.cend(), shader_names.cbegin()))
        {
            LOG_ERROR("Redefinition of already existing program '", name,"'");
            abort();
        }
        return;
    }

    it = m_pipeline_names.find(name);
    if (it != m_pipeline_names.end()) {
        LOG_ERROR("Program pipeline object with same name already in shader manager: ", name);
        abort();
    }

    // check, if the same program (with different name) already exists
    std::size_t idx;
    for (idx = 0; idx < m_programs.size(); ++idx) {
        const auto& info = m_programs[idx];
        if (info.is_separable == is_separable &&
            info.shaders.size() == shader_names.size() &&
            std::equal(info.shaders.cbegin(), info.shaders.cend(),
                       shader_names.cbegin()))
        {
            break;
        }
    }

    if (idx == m_programs.size()) {
        for (const auto& s : shader_names) {
            if (m_shader_names.find(s) == m_shader_names.end()) {
                LOG_ERROR("Can't find shader '", s, '\'');
                abort();
            }
        }

        m_programs.emplace_back();
        auto& info = m_programs.back();
        info.shaders = std::move(shader_names);
        info.is_separable = is_separable;
    }

    m_program_names.emplace(name, idx);
}

/****************************************************************************/

void ShaderManager::registerProgramPipeline(const std::string& name,
        std::initializer_list<std::string> program_names)
{
    std::vector<std::string> programs(program_names);
    std::sort(programs.begin(), programs.end());

    auto it = m_pipeline_names.find(name);
    if (it != m_pipeline_names.end()) {
        const auto& other = m_pipelines[it->second];
        if (programs.size() != other.programs.size() ||
            !std::equal(programs.cbegin(), programs.cend(),
                        other.programs.cbegin()))
        {
            LOG_ERROR("Redefinition of already existing program pipeline '", name,"'");
            abort();
        }
        return;
    }

    it = m_program_names.find(name);
    if (it != m_program_names.end()) {
        LOG_ERROR("Program with same name already in shader manager: ", name);
        abort();
    }

    // check, if the same pipeline (with different name) already exists
    std::size_t idx;
    for (idx = 0; idx < m_pipelines.size(); ++idx) {
        const auto& other = m_pipelines[idx];
        if (programs.size() == other.programs.size() &&
            std::equal(programs.cbegin(), programs.cend(),
                       other.programs.cbegin()))
        {
            break;
        }
    }

    if (idx == m_pipelines.size()) {
        for (const auto& prog : programs) {
            if (m_program_names.find(prog) == m_program_names.end()) {
                LOG_ERROR("Can't find program '", prog, '\'');
                abort();
            }
        }

        m_pipelines.emplace_back();
        auto& info = m_pipelines.back();
        info.programs = std::move(programs);
    }

    m_program_names.emplace(name, idx);
}

/****************************************************************************/

void ShaderManager::setDefines(const std::string& defines)
{
    m_defines.clear();
    m_defines_str.clear();
    addDefines(defines);
}

/****************************************************************************/

void ShaderManager::addDefines(const std::string& defines)
{
    if (defines.empty())
        return;

    const std::size_t prev_size = m_defines.size();

    boost::char_separator<char> sep(",");
    boost::tokenizer<boost::char_separator<char>> tokens(defines, sep);
    for (const auto& d : tokens) {
        if (std::find(m_defines.begin(), m_defines.end(), d) != m_defines.end())
            continue;
        m_defines.push_back(d);
    }

    for (size_t i = prev_size; i < m_defines.size(); ++i) {
        m_defines_str += ',' + m_defines[i];
    }
}

/****************************************************************************/

void ShaderManager::releaseShaders()
{
    for (auto& s : m_shaders) {
        if (s.shader.get() != 0) {
            s.shader = gl::Shader();
        }
    }
    glReleaseShaderCompiler();
}

/****************************************************************************/

bool ShaderManager::recompile()
{
    bool result = true;

    for (auto& shader : m_shaders) {
        if (shader.shader.get() == 0)
            continue;
        for (const auto& file : shader.files) {
            if (m_timestamp < last_write_time(file)) {
                result &= compile_shader(shader);
                break;
            }
        }
    }

    for (auto& prog : m_programs) {
        if (prog.program.get() == 0)
            continue;
        for (const auto& file : prog.files) {
            if (m_timestamp < last_write_time(file)) {
                result &= link_program(prog, false);
                break;
            }
        }
    }

    for (auto& pipeline : m_pipelines) {
        if (pipeline.pipeline.get() == 0)
            continue;
        setup_pipeline(pipeline);
    }

    m_timestamp = std::time(nullptr);
    return result;
}

/****************************************************************************/

bool ShaderManager::preloadAll()
{
    bool result = true;

    for (auto& prog : m_programs) {
        if (prog.program.get() == 0) {
            result &= link_program(prog, true);
        }
    }
    for (auto& pipeline : m_pipelines) {
        setup_pipeline(pipeline);
    }

    return result;
}

/****************************************************************************/

bool ShaderManager::compile_shader(ShaderInfo& info) const
{
    std::string defines = m_defines_str;
    if (defines.empty() && !info.defines.empty()) {
        defines += ',';
    }
    defines += info.defines;
    if (defines.empty() == false)
        defines += ',';
    switch (info.type) {
    case GL_VERTEX_SHADER:
        defines += "VERTEX_SHADER";
        break;
    case GL_TESS_CONTROL_SHADER:
        defines += "TESS_CONTROL_SHADER";
        break;
    case GL_TESS_EVALUATION_SHADER:
        defines += "TESS_EVALUATION_SHADER";
        break;
    case GL_GEOMETRY_SHADER:
        defines += "GEOMETRY_SHADER";
        break;
    case GL_FRAGMENT_SHADER:
        defines += "FRAGMENT_SHADER";
        break;
    case GL_COMPUTE_SHADER:
        defines += "COMPUTE_SHADER";
        break;
    default:
        abort();
    }

    gl::ShaderSource source(info.filename, defines);

    if (!source) {
        return false;
    }

    gl::Shader shader(info.type);
    glShaderSource(shader, 1, source.sourcePtr(), nullptr);
    glCompileShader(shader);
    if (!shader) {
        std::ostringstream msg;
        msg << "Failed to compile shader!\nFILES:\n";
        int i = 0;
        for (const auto& f : source.filenames()) {
            msg << " (" << i++ << ") " << f << '\n';
        }
        msg << "Info Log:\n" << shader.getInfoLog();

        LOG_ERROR(logtag::OpenGL, msg.str());

        return false;
    }

    info.shader = std::move(shader);
    info.files = std::move(source.filenames());

    return true;
}

/****************************************************************************/

bool ShaderManager::link_program(ProgramInfo& p, const bool use_cache) const
{
    const std::string id = program_id(p);

    if (use_cache && load_cached_program(p, id))
        return true;

    // manually link
    gl::Program prog;
    glProgramParameteri(prog, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
    if (p.is_separable)
        glProgramParameteri(prog, GL_PROGRAM_SEPARABLE, GL_TRUE);
    for (const auto& shader : p.shaders) {
        auto it = m_shader_names.find(shader);
        if (it == m_shader_names.end()) {
            LOG_ERROR("Unable to link program: Unkown shader '",
                    shader, ',');
            return false;
        }
        auto& s = m_shaders[it->second];
        if (s.shader.get() == 0) {
            compile_shader(s);
        }
        if (!s.shader) {
            LOG_ERROR("Unable to link program: Shader '", shader,
                    "' failed to compile");
            return false;
        }
        glAttachShader(prog, s.shader);
    }
    glLinkProgram(prog);
    if (!prog) {
        LOG_ERROR("Failed to link program: ", prog.getInfoLog());
        return false;
    }

    // detach shaders and update list of filenames
    p.files.clear();
    for (const auto& shader : p.shaders) {
        auto it = m_shader_names.find(shader);
        auto& s = m_shaders[it->second];

        glDetachShader(prog, s.shader);
        p.files.insert(p.files.end(), s.files.begin(), s.files.end());
    }
    std::sort(p.files.begin(), p.files.end());
    p.files.erase(std::unique(p.files.begin(), p. files.end()), p.files.end());

    save_program(p, id);

    return true;
}

/****************************************************************************/

std::string ShaderManager::program_id(const ProgramInfo& p) const
{
    std::string id;

    id += reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    id += reinterpret_cast<const char*>(glGetString(GL_VERSION));
    id += reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    id += reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    id += '_' + std::to_string(p.shaders.size()) + '_';
    for (std::size_t i = 0; i < p.shaders.size(); ++i) {
        const auto& name = p.shaders[i];
        const auto& info = m_shaders[m_shader_names.find(name)->second];
        id += info.defines + '%';
        id += info.filename + '_';
    }

    return id;
}

/****************************************************************************/

bool ShaderManager::load_cached_program(ProgramInfo& p, const std::string& id) const
{
    const path cache_file = vars.cache_dir / std::to_string(m_hasher(id));

    if (!exists(cache_file) || !is_regular_file(cache_file)) {
        return false;
    }

    std::ifstream is(cache_file.c_str(), std::ios::in | std::ios::binary);
    if (!is)
        return false;

    is.seekg(0, std::ios::end);
    const auto file_size = static_cast<std::size_t>(is.tellg());
    is.seekg(0, std::ios::beg);

    if (file_size <= sizeof(CacheFile))
        return false;

    std::unique_ptr<char[]> data{new char[file_size + 1]};
    is.read(data.get(), static_cast<long>(file_size));
    is.close();
    data[file_size] = '\0';

    const CacheFile* header = reinterpret_cast<CacheFile*>(data.get());
    const char* ptr = data.get() + sizeof(CacheFile);
    const char* end = data.get() + file_size;

    if (header->num_shaders != static_cast<int>(p.shaders.size()) ||
        header->is_separable != p.is_separable)
    {
        std::remove(cache_file.c_str());
        return false;
    }

    // check, if cached program uses registered shaders
    bool valid = true;
    for (std::size_t i = 0; i < p.shaders.size(); ++i) {
        const auto& name = p.shaders[i];
        auto it = m_shader_names.find(name);
        if (it == m_shader_names.end()) {
            valid = false;
            break;
        }
        const auto& info = m_shaders[it->second];
        if (std::strcmp(ptr, info.filename.c_str()) != 0) {
            valid = false;
            break;
        }
        ptr += info.filename.size() + 1;
        assert(ptr < end);
        if (std::strcmp(ptr, info.defines.c_str()) != 0) {
            valid = false;
            break;
        }
        ptr += info.defines.size() + 1;
        assert(ptr < end);
    }
    if (!valid) {
        std::remove(cache_file.c_str());
        return false;
    }

    // read filenames
    std::vector<std::string> filenames;
    filenames.reserve(static_cast<std::size_t>(header->num_files));
    for (int i = 0; i < header->num_files; ++i) {
        filenames.emplace_back(ptr);
        ptr += filenames.back().size() + 1;
        assert(ptr < end);
    }

    // check time of last modification on files
    const auto last_write = last_write_time(cache_file);
    for (const auto& f : filenames) {
        if (last_write < last_write_time(f)) {
            valid = false;
            break;
        }
    }
    if (!valid) {
        std::remove(cache_file.c_str());
        return false;
    }

    gl::Program prog;
    glProgramParameteri(prog, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
    glProgramBinary(prog, header->binary_format, ptr,
            static_cast<GLsizei>(end - ptr));
    if (!prog)
        return false;

    p.files = std::move(filenames);
    p.program = std::move(prog);

    return true;
}

/****************************************************************************/

void ShaderManager::save_program(const ProgramInfo& info, const std::string& id) const
{
    path cache_file = vars.cache_dir / std::to_string(m_hasher(id));

    std::ofstream os(cache_file.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!os)
        return;

    CacheFile header;

    GLint binary_size;
    glGetProgramiv(info.program, GL_PROGRAM_BINARY_LENGTH, &binary_size);
    std::unique_ptr<char[]> data{new char[binary_size]};
    glGetProgramBinary(info.program, binary_size, nullptr, &header.binary_format, data.get());

    header.is_separable = info.is_separable;
    header.num_files = static_cast<int>(info.files.size());
    header.num_shaders = static_cast<int>(info.shaders.size());

    os.write(reinterpret_cast<const char*>(&header), sizeof(CacheFile));

    for (const auto& name : info.shaders) {
        const auto& shader = m_shaders[m_shader_names.find(name)->second];
        const auto& filename = shader.filename;
        const auto& defines = shader.defines;
        os.write(filename.c_str(), static_cast<long>(filename.size() + 1));
        os.write(defines.c_str(), static_cast<long>(defines.size() + 1));
    }
    for (const auto& file : info.files) {
        os.write(file.c_str(), static_cast<long>(file.size() + 1));
    }
    os.write(data.get(), binary_size);
}

/****************************************************************************/

void ShaderManager::setup_pipeline(PipelineInfo& p) const
{
    gl::ProgramPipeline pipeline = (p.pipeline.get() != 0) ?
            std::move(p.pipeline) : gl::ProgramPipeline();

    glUseProgramStages(pipeline, GL_ALL_SHADER_BITS, 0);

    for (const auto& prog_name : p.programs) {
        ProgramInfo& prog = m_programs[m_program_names.find(prog_name)->second];

        if (prog.program.get() == 0) {
            link_program(prog, true);
        }
        if (!prog.program) {
            LOG_ERROR("Failed to setup program pipeline: ", prog_name,
                    " failed to compile/link");
            return;
        }

        GLbitfield stages = 0;
        for (const auto& shader_name : prog.shaders) {
            const auto& info = m_shaders[m_shader_names.find(shader_name)->second];
            switch (info.type) {
            case GL_VERTEX_SHADER:
                stages |= GL_VERTEX_SHADER_BIT;
                break;
            case GL_TESS_CONTROL_SHADER:
                stages |= GL_TESS_CONTROL_SHADER_BIT;
                break;
            case GL_TESS_EVALUATION_SHADER:
                stages |= GL_TESS_EVALUATION_SHADER_BIT;
                break;
            case GL_GEOMETRY_SHADER:
                stages |= GL_GEOMETRY_SHADER_BIT;
                break;
            case GL_FRAGMENT_SHADER:
                stages |= GL_FRAGMENT_SHADER_BIT;
                break;
            case GL_COMPUTE_SHADER:
                stages |= GL_COMPUTE_SHADER_BIT;
            default:
                abort();
            }
        }
        glUseProgramStages(pipeline, stages, prog.program);
    }

    p.pipeline = std::move(pipeline);
}

/****************************************************************************/

} // namespace core
