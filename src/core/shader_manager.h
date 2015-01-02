#ifndef CORE_SHADER_MANAGER_H
#define CORE_SHADER_MANAGER_H

#include <ctime>
#include <unordered_map>
#include <vector>
#include <string>
#include <initializer_list>

#include "gl/opengl.h"
#include "managers.h"
#include "shader.h"
#include "program.h"
#include "program_pipeline.h"

namespace core
{

/****************************************************************************/

class ShaderManager
{
public:
    ShaderManager();
    ~ShaderManager();

    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;

    Shader registerShader(const std::string& name, const std::string& filename,
            GLenum shader_type, std::string defines = "");

    Program registerProgram(const std::string& name,
            std::initializer_list<std::string> shaders,
            bool is_separable = false);

    ProgramPipeline registerProgramPipeline(const std::string& name,
            std::initializer_list<std::string> programs);

    Shader getShader(const std::string& name) const;

    Program getProgram(const std::string& name) const;

    ProgramPipeline getProgramPipeline(const std::string& name) const;

    void setDefines(const std::string& defines);
    void addDefines(const std::string& defines);

    void releaseShaders();
    bool recompile();

    bool preloadAll();

private:
    friend class Shader;
    friend class Program;
    friend class ProgramPipeline;

    const gl::Shader& getShader(std::size_t idx) const;
    const gl::Program& getProgram(std::size_t idx) const;
    const gl::ProgramPipeline& getProgramPipeline(std::size_t idx) const;


    struct ShaderInfo;
    struct ProgramInfo;
    struct PipelineInfo;

    bool compile_shader(ShaderInfo& s) const;
    bool link_program(ProgramInfo& p, bool use_cache) const;
    std::string program_id(const ProgramInfo& p) const;
    bool load_cached_program(ProgramInfo& p, const std::string& id) const;
    void save_program(const ProgramInfo& p, const std::string& id) const;
    void setup_pipeline(PipelineInfo& p) const;

    std::vector<std::string>                        m_defines;
    std::string                                     m_defines_str;
    std::unordered_map<std::string, std::size_t>    m_shader_names;
    std::unordered_map<std::string, std::size_t>    m_program_names;
    std::unordered_map<std::string, std::size_t>    m_pipeline_names;
    mutable std::vector<ShaderInfo>                 m_shaders;
    mutable std::vector<ProgramInfo>                m_programs;
    mutable std::vector<PipelineInfo>               m_pipelines;
    std::hash<std::string>                          m_hasher;
    std::time_t                                     m_timestamp;

};

} // namespace core

#endif // CORE_SHADER_MANAGER_H

