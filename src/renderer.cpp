#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

#include "renderer.h"
#include "core/mesh.h"
#include "core/shader_manager.h"
#include "core/mesh_manager.h"
#include "core/instance_manager.h"
#include "core/camera_manager.h"
#include "core/material_manager.h"
#include "core/shader_interface.h"
#include "core/texture.h"
#include "log/log.h"

/****************************************************************************/

struct Renderer::DrawCmd
{
    DrawCmd(const core::Instance* instance_, core::Program prog_,
            GLuint vao_, GLenum mode_, GLsizei count_, GLenum type_,
            GLvoid* indices_, GLint basevertex_)
      : instance{instance_},
        prog{prog_},
        vao{vao_},
        mode{mode_},
        count{count_},
        type{type_},
        indices{indices_},
        basevertex{basevertex_}
    {
    }

    const core::Instance*   instance;
    core::Program           prog;
    GLuint                  vao;
    GLenum                  mode;
    GLsizei                 count;
    GLenum                  type;
    GLvoid*                 indices;
    GLint                   basevertex;
};

/****************************************************************************/

Renderer::Renderer()
{
    // create programs
    for (unsigned char i = 0; i < 8; ++i) {
        util::bitfield<core::MeshComponents> c(i);
        std::string name_ext = "pos";
        std::string defines;
        if (c & core::MeshComponents::Normals) {
            name_ext += "_normals";
            defines = "HAS_NORMALS";
        }
        if (c & core::MeshComponents::TexCoords) {
            name_ext += "_texcoords";
            if (!defines.empty())
                defines += ',';
            defines += "HAS_TEXCOORDS";
        }
        if (c & core::MeshComponents::Tangents) {
            name_ext += "_tangents";
            if (!defines.empty())
                defines += ',';
            defines += "HAS_TANGENTS";
        }
        const auto vert = "basic_vert_" + name_ext;
        const auto frag = "basic_frag_" + name_ext;
        core::res::shaders->registerShader(vert, "basic/basic.vert", GL_VERTEX_SHADER,
                defines);
        core::res::shaders->registerShader(frag, "basic/basic.frag", GL_FRAGMENT_SHADER,
                defines);
        auto prog = core::res::shaders->registerProgram("prog_" + vert + frag,
                {vert, frag});
        m_programs.emplace(c(), prog);
    }
}

/****************************************************************************/

Renderer::~Renderer() = default;

/****************************************************************************/

void Renderer::setGeometry(std::vector<const core::Instance*> geometry)
{
    m_geometry = std::move(geometry);

    std::sort(m_geometry.begin(), m_geometry.end(),
            [] (const core::Instance* g0, const core::Instance* g1) -> bool
            {
                return g0->getMesh()->components() < g1->getMesh()->components();
            });

    m_drawlist.clear();
    m_drawlist.reserve(m_geometry.size());
    for (const auto* g : m_geometry) {
        const auto* mesh = g->getMesh();
        const auto prog = m_programs[mesh->components()()];
        GLuint vao = core::res::meshes->getVAO(mesh);
        m_drawlist.emplace_back(g, prog, vao, mesh->mode(),
                mesh->count(), mesh->type(),
                mesh->indices(), mesh->basevertex());
    }
}

/****************************************************************************/

void Renderer::render()
{
    if (m_geometry.empty())
        return;

    core::res::materials->bind();

    //const auto* cam = core::res::cameras->getDefaultCam();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    GLuint prog = 0;
    GLuint vao = 0;

    GLuint textures[core::bindings::NUM_TEXT_UNITS] = {0,};

    for (const auto& cmd : m_drawlist) {
        //if (!cam->inFrustum(cmd.instance->getBoundingBox()))
        //    continue;
        if (prog != cmd.prog) {
            prog = cmd.prog;
            glUseProgram(prog);
        }
        if (vao != cmd.vao) {
            vao = cmd.vao;
            glBindVertexArray(vao);
        }

        // bind textures
        const auto* mat = cmd.instance->getMaterial();
        if (mat->hasDiffuseTexture()) {
            int unit = core::bindings::DIFFUSE_TEX_UNIT;
            GLuint tex = *mat->getDiffuseTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(GL_TEXTURE_2D, tex);
                //glBindMultiTextureEXT(unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasSpecularTexture()) {
            int unit = core::bindings::SPECULAR_TEX_UNIT;
            GLuint tex = *mat->getSpecularTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(GL_TEXTURE_2D, tex);
                //glBindMultiTextureEXT(unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasGlossyTexture()) {
            int unit = core::bindings::GLOSSY_TEX_UNIT;
            GLuint tex = *mat->getGlossyTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(GL_TEXTURE_2D, tex);
                //glBindMultiTextureEXT(unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasNormalTexture()) {
            int unit = core::bindings::NORMAL_TEX_UNIT;
            GLuint tex = *mat->getNormalTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(GL_TEXTURE_2D, tex);
                //glBindMultiTextureEXT(unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasEmissiveTexture()) {
            int unit = core::bindings::EMISSIVE_TEX_UNIT;
            GLuint tex = *mat->getEmissiveTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(GL_TEXTURE_2D, tex);
                //glBindMultiTextureEXT(unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasAlphaTexture()) {
            int unit = core::bindings::ALPHA_TEX_UNIT;
            GLuint tex = *mat->getAlphaTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(GL_TEXTURE_2D, tex);
                //glBindMultiTextureEXT(unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasAmbientTexture()) {
            int unit = core::bindings::AMBIENT_TEX_UNIT;
            GLuint tex = *mat->getAmbientTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(GL_TEXTURE_2D, tex);
                //glBindMultiTextureEXT(unit, GL_TEXTURE_2D, tex);
            }
        }

        glm::mat4 id(1.f);
        GLint loc = glGetUniformLocation(prog, "uModelMatrix");
        glUniformMatrix4fv(loc, 1, GL_FALSE,
                glm::value_ptr(cmd.instance->getTransformationMatrix()));

        loc = glGetUniformLocation(prog, "uMaterialID");
        glUniform1ui(loc, mat->getIndex());

        glDrawElementsBaseVertex(cmd.mode, cmd.count,
                cmd.type, cmd.indices, cmd.basevertex);
    }
}

/****************************************************************************/
