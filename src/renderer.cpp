#include <cassert>
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
#include "framework/vars.h"

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
    /*for (unsigned char i = 0; i < 8; ++i) {
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
    core::res::shaders->registerShader("noop_frag", "basic/noop.frag", GL_FRAGMENT_SHADER);
    m_earlyz_prog = core::res::shaders->registerProgram("early_z_prog",
            {"basic_vert_pos", "noop_frag"});*/

    initBBoxStuff();

    // programmable vertex pulling
    core::res::shaders->registerShader("vertexpulling_vert", "basic/vertexpulling.vert",
            GL_VERTEX_SHADER);
    core::res::shaders->registerShader("vertexpulling_frag", "basic/vertexpulling.frag",
            GL_FRAGMENT_SHADER);
    m_vertexpulling_prog = core::res::shaders->registerProgram("vertexpulling_prog",
            {"vertexpulling_vert", "vertexpulling_frag"});

    // voxel stuff
    core::res::shaders->registerShader("voxelGeom", "tree/trianglemod.geom", GL_GEOMETRY_SHADER);
    core::res::shaders->registerShader("voxelFrag", "tree/trianglemod.frag", GL_FRAGMENT_SHADER);
    m_voxel_prog = core::res::shaders->registerProgram("voxel_prog",
            {"vertexpulling_vert", "voxelGeom", "voxelFrag"});

    glBindVertexArray(m_vertexpulling_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, core::res::meshes->getElementArrayBuffer());
    glBindVertexArray(0);
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

void genAtomicBuffer(int num, unsigned int & buffer) {

    GLuint initVal = 0;

    if(buffer)
        glDeleteBuffers(1, &buffer);
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, buffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &initVal, GL_STATIC_DRAW);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

}
GLuint atomicBuffer = 0;
void Renderer::render(const bool renderBBoxes)
{
    if (m_geometry.empty())
        return;

    core::res::materials->bind();
    core::res::instances->bind();
    core::res::meshes->bind();

    const auto* cam = core::res::cameras->getDefaultCam();

    if (true) {

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDepthFunc(GL_LEQUAL);

        glUseProgram(m_voxel_prog);

        GLint loc = glGetUniformLocation(m_voxel_prog, "u_width");
        glUniform1i(loc, vars.screen_width);
        loc = glGetUniformLocation(m_voxel_prog, "u_height");
        glUniform1i(loc, vars.screen_height);

        genAtomicBuffer(1, atomicBuffer);

        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, atomicBuffer);

        //Create an modelview-orthographic projection matrix see from X/Y/Z axis
        glm::mat4 Ortho;
        Ortho = glm::ortho( -1.0f, 1.0f, -1.0f, 1.0f, 2.0f-1.0f, 3.0f );
        glm::mat4 mvpX = Ortho * glm::lookAt( glm::vec3( 2, 0, 0 ), glm::vec3( 0, 0, 0 ), glm::vec3( 0, 1, 0 ) );
        glm::mat4 mvpY = Ortho * glm::lookAt( glm::vec3( 0, 2, 0 ), glm::vec3( 0, 0, 0 ), glm::vec3( 0, 0, -1 ) );
        glm::mat4 mvpZ = Ortho * glm::lookAt( glm::vec3( 0, 0, 2 ), glm::vec3( 0, 0, 0 ), glm::vec3( 0, 1, 0 ) );

        loc = glGetUniformLocation(m_voxel_prog, "u_MVPx");
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mvpX));
        loc = glGetUniformLocation(m_voxel_prog, "u_MVPy");
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mvpY));
        loc = glGetUniformLocation(m_voxel_prog, "u_MVPz");
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mvpZ));

        // texture stuff
        const unsigned int tex_size = 512;

        gl::Texture tex_voxel_pos;
        gl::Texture tex_voxel_kd;
        gl::Texture tex_voxel_normal;

        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB10_A2UI, tex_size, tex_size);
        glActiveTexture(tex_voxel_pos);
        glBindImageTexture(0, tex_voxel_pos, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGB10_A2UI);

        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, tex_size, tex_size);
        glActiveTexture(tex_voxel_kd);
        glBindImageTexture(1, tex_voxel_kd, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, tex_size, tex_size);
        glActiveTexture(tex_voxel_normal);
        glBindImageTexture(2, tex_voxel_normal, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

        // render
        glBindVertexArray(m_vertexpulling_vao);
        for (const auto& cmd : m_drawlist) {
            // Frustum Culling
            if (!cam->inFrustum(cmd.instance->getBoundingBox()))
                continue;

            glDrawElementsInstancedBaseVertexBaseInstance(cmd.mode, cmd.count, cmd.type,
                    cmd.indices, 1, 0, cmd.instance->getIndex());
        }

        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicBuffer);
        auto count = static_cast<GLuint *>(glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));
        LOG_INFO("num written pixels: ", *count);

        return;

    }



    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);

    //GLuint prog = m_earlyz_prog;
    //GLuint vao = 0;
    GLuint textures[core::bindings::NUM_TEXT_UNITS] = {0,};

    /*
    // early z
    glUseProgram(prog);
    for (const auto& cmd : m_drawlist) {
        // Frustum Culling
        if (!cam->inFrustum(cmd.instance->getBoundingBox()))
            continue;

        // only fully opaque meshes:
        const auto* mat = cmd.instance->getMaterial();
        if (mat->hasAlphaTexture() || mat->getOpacity() != 1.f)
            continue;

        if (vao != cmd.vao) {
            vao = cmd.vao;
            glBindVertexArray(vao);
        }

        glDrawElementsInstancedBaseVertexBaseInstance(cmd.mode, cmd.count, cmd.type,
                cmd.indices, 1, cmd.basevertex, cmd.instance->getIndex());
    }
    */

    glUseProgram(m_vertexpulling_prog);
    glBindVertexArray(m_vertexpulling_vao);
    for (const auto& cmd : m_drawlist) {
        // Frustum Culling
        if (!cam->inFrustum(cmd.instance->getBoundingBox()))
            continue;

        //if (prog != cmd.prog) {
        //    prog = cmd.prog;
        //    glUseProgram(prog);
        //}
        //if (vao != cmd.vao) {
        //    vao = cmd.vao;
        //    glBindVertexArray(vao);
        //}

        // bind textures
        const auto* mat = cmd.instance->getMaterial();
        if (mat->hasDiffuseTexture()) {
            int unit = core::bindings::DIFFUSE_TEX_UNIT;
            GLuint tex = *mat->getDiffuseTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasSpecularTexture()) {
            int unit = core::bindings::SPECULAR_TEX_UNIT;
            GLuint tex = *mat->getSpecularTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasGlossyTexture()) {
            int unit = core::bindings::GLOSSY_TEX_UNIT;
            GLuint tex = *mat->getGlossyTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasNormalTexture()) {
            int unit = core::bindings::NORMAL_TEX_UNIT;
            GLuint tex = *mat->getNormalTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasEmissiveTexture()) {
            int unit = core::bindings::EMISSIVE_TEX_UNIT;
            GLuint tex = *mat->getEmissiveTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasAlphaTexture()) {
            int unit = core::bindings::ALPHA_TEX_UNIT;
            GLuint tex = *mat->getAlphaTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasAmbientTexture()) {
            int unit = core::bindings::AMBIENT_TEX_UNIT;
            GLuint tex = *mat->getAmbientTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }

        //glDrawElementsInstancedBaseVertexBaseInstance(cmd.mode, cmd.count, cmd.type,
        //        cmd.indices, 1, cmd.basevertex, cmd.instance->getIndex());
        glDrawElementsInstancedBaseVertexBaseInstance(cmd.mode, cmd.count, cmd.type,
                cmd.indices, 1, 0, cmd.instance->getIndex());
    }

    if (renderBBoxes)
        renderBoundingBoxes();
}

/****************************************************************************/

void Renderer::renderBoundingBoxes()
{
    if (m_geometry.empty())
        return;

    core::res::instances->bind();

    GLuint prog = m_bbox_prog;
    glUseProgram(prog);
    glBindVertexArray(m_bbox_vao);

    glDisable(GL_DEPTH_TEST);

    for (const auto* g : m_geometry) {
        glDrawElementsInstancedBaseVertexBaseInstance(GL_LINES, 24, GL_UNSIGNED_BYTE,
                nullptr, 1, g->getMesh()->basevertex(), g->getIndex());
    }
}

/****************************************************************************/

void Renderer::initBBoxStuff()
{
    // indices
    GLubyte indices[] = {0, 1,
                         0, 2,
                         0, 4,
                         1, 3,
                         1, 5,
                         2, 3,
                         2, 6,
                         3, 7,
                         4, 5,
                         4, 6,
                         5, 7,
                         6, 7};

    glBindVertexArray(m_bbox_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bbox_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindVertexArray(0);

    core::res::shaders->registerShader("bbox_vert", "basic/bbox.vert", GL_VERTEX_SHADER);
    core::res::shaders->registerShader("bbox_frag", "basic/bbox.frag", GL_FRAGMENT_SHADER);
    m_bbox_prog = core::res::shaders->registerProgram("bbox_prog", {"bbox_vert", "bbox_frag"});
}

/****************************************************************************/
