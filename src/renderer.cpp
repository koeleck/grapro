#include <glm/gtc/type_ptr.hpp>

#include "renderer.h"
#include "core/mesh.h"
#include "core/shader_manager.h"
#include "core/mesh_manager.h"
#include "core/instance_manager.h"
#include "core/camera_manager.h"
#include "core/material_manager.h"
#include "core/texture_manager.h"
#include "log/log.h"

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
}

/****************************************************************************/

void Renderer::render()
{
    if (m_geometry.empty())
        return;

    core::res::materials->bind();
    core::res::textures->bind();

    //const auto* tex = core::res::textures->getTexture("textures/spnza_bricks_a_diff.tga");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    for (auto g : m_geometry) {
        const auto* mesh = g->getMesh();
        GLuint prog = m_programs[mesh->components()()];
        GLuint vao = core::res::meshes->getVAO(mesh);
        glUseProgram(prog);
        glBindVertexArray(vao);

        glm::mat4 id(1.f);
        GLint loc = glGetUniformLocation(prog, "uModelMatrix");
        glUniformMatrix4fv(loc, 1, GL_FALSE,
                glm::value_ptr(g->getTransformationMatrix()));

        loc = glGetUniformLocation(prog, "uMaterialID");
        glUniform1ui(loc, g->getMaterial()->getIndex());

        //loc = glGetUniformLocation(prog, "uTexHandle");
        //glUniformHandleui64ARB(loc, tex->getTextureHandle());

        glDrawElementsBaseVertex(mesh->mode(), mesh->count(),
                mesh->type(), mesh->indices(), mesh->basevertex());
        //LOG_INFO("prog: ", prog, ", vao: ", vao);
        //LOG_INFO("glDrawElementsBaseVertex(", mesh->mode(), ", ",  mesh->count(),
        //        ", ", mesh->type(), ", ", mesh->indices(),
        //        ", ", mesh->basevertex(), ");");
    }
    //LOG_INFO("----------------------");
}

/****************************************************************************/
