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

/****************************************************************************/

namespace
{

typedef struct
{
    GLuint  count;
    GLuint  instanceCount;
    GLuint  firstIndex;
    GLuint  baseVertex;
    GLuint  baseInstance;
} DrawElementsIndirectCommand;

} // anonymous namespace

/****************************************************************************/

struct Renderer::DrawCmd
{
    DrawCmd(const GLenum mode_, const GLenum type_, const GLvoid* const indirect_,
            const GLsizei drawcount_, const GLsizei stride_)
      : textures{0,},
        mode{mode_},
        type{type_},
        indirect{indirect_},
        drawcount{drawcount_},
        stride{stride_}
    {
    }

    GLuint                  textures[core::bindings::NUM_TEXT_UNITS];
    GLenum                  mode;
    GLenum                  type;
    const GLvoid*           indirect;
    GLsizei                 drawcount;
    GLsizei                 stride;
    core::AABB              aabb;
};

/****************************************************************************/

Renderer::Renderer()
{
    // early-z
    //core::res::shaders->registerShader("noop_frag", "basic/noop.frag", GL_FRAGMENT_SHADER);
    //m_earlyz_prog = core::res::shaders->registerProgram("early_z_prog",
    //        {"basic_vert_pos", "noop_frag"});

    initBBoxStuff();

    // programmable vertex pulling
    core::res::shaders->registerShader("vertexpulling_vert", "basic/vertexpulling.vert",
            GL_VERTEX_SHADER);
    core::res::shaders->registerShader("vertexpulling_frag", "basic/vertexpulling.frag",
            GL_FRAGMENT_SHADER);
    m_vertexpulling_prog = core::res::shaders->registerProgram("vertexpulling_prog",
            {"vertexpulling_vert", "vertexpulling_frag"});

    glBindVertexArray(m_vertexpulling_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, core::res::meshes->getElementArrayBuffer());
    glBindVertexArray(0);

    core::res::shaders->registerShader("downsample_depth_comp", "basic/downsample_depth.comp",
            GL_COMPUTE_SHADER);
    m_oc_downsample_prog = core::res::shaders->registerProgram("downsample_depth_prog",
            {"downsample_depth_comp"});
}

/****************************************************************************/

Renderer::~Renderer() = default;

/****************************************************************************/

void Renderer::setGeometry(std::vector<const core::Instance*> geometry)
{
    m_geometry = std::move(geometry);

    // sort by textures used
    std::sort(m_geometry.begin(), m_geometry.end(),
            [] (const core::Instance* g0, const core::Instance* g1) -> bool
            {
                const auto* mat0 = g0->getMaterial();
                const auto* mat1 = g1->getMaterial();

                if (mat0 == mat1)
                    return g0->getMesh()->type() < g1->getMesh()->type();
                return mat0 < mat1;
            });

    // Create draw calls
    m_drawlist.clear();
    m_drawlist.reserve(m_geometry.size());

    std::vector<GLuint> instanceIDs;
    instanceIDs.reserve(m_geometry.size());

    std::size_t indirect = 0;
    m_drawlist.emplace_back(GL_TRIANGLES,
            m_geometry.front()->getMesh()->type(),
            reinterpret_cast<const GLvoid*>(indirect), 0, 0);
    const auto* prev_mat = m_geometry.front()->getMaterial();
    for (const auto* g : m_geometry) {
        instanceIDs.emplace_back(g->getIndex());

        const auto* mesh = g->getMesh();
        const auto* mat = g->getMaterial();
        if (mesh->type() != m_drawlist.back().type || mat != prev_mat) {
            m_drawlist.emplace_back(GL_TRIANGLES, mesh->type(),
                    reinterpret_cast<const GLvoid*>(indirect), 0, 0);
        }
        prev_mat = mat;
        {
            auto& cmd = m_drawlist.back();
            cmd.drawcount++;
            cmd.aabb.expandBy(g->getBoundingBox());
            if (mat->hasDiffuseTexture()) {
                const auto unit = core::bindings::DIFFUSE_TEX_UNIT;
                const GLuint tex = *mat->getDiffuseTexture();
                assert(cmd.textures[unit] == 0 || cmd.textures[unit] == tex);
                cmd.textures[unit] = tex;
            }
            if (mat->hasSpecularTexture()) {
                const auto unit = core::bindings::SPECULAR_TEX_UNIT;
                const GLuint tex = *mat->getSpecularTexture();
                assert(cmd.textures[unit] == 0 || cmd.textures[unit] == tex);
                cmd.textures[unit] = tex;
            }
            if (mat->hasEmissiveTexture()) {
                const auto unit = core::bindings::EMISSIVE_TEX_UNIT;
                const GLuint tex = *mat->getEmissiveTexture();
                assert(cmd.textures[unit] == 0 || cmd.textures[unit] == tex);
                cmd.textures[unit] = tex;
            }
            if (mat->hasGlossyTexture()) {
                const auto unit = core::bindings::GLOSSY_TEX_UNIT;
                const GLuint tex = *mat->getGlossyTexture();
                assert(cmd.textures[unit] == 0 || cmd.textures[unit] == tex);
                cmd.textures[unit] = tex;
            }
            if (mat->hasAlphaTexture()) {
                const auto unit = core::bindings::ALPHA_TEX_UNIT;
                const GLuint tex = *mat->getAlphaTexture();
                assert(cmd.textures[unit] == 0 || cmd.textures[unit] == tex);
                cmd.textures[unit] = tex;
            }
            if (mat->hasNormalTexture()) {
                const auto unit = core::bindings::NORMAL_TEX_UNIT;
                const GLuint tex = *mat->getNormalTexture();
                assert(cmd.textures[unit] == 0 || cmd.textures[unit] == tex);
                cmd.textures[unit] = tex;
            }
            if (mat->hasAmbientTexture()) {
                const auto unit = core::bindings::AMBIENT_TEX_UNIT;
                const GLuint tex = *mat->getAmbientTexture();
                assert(cmd.textures[unit] == 0 || cmd.textures[unit] == tex);
                cmd.textures[unit] = tex;
            }
        }

        indirect += sizeof(DrawElementsIndirectCommand);
    }

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);
    glBufferData(GL_DRAW_INDIRECT_BUFFER,
            static_cast<GLsizeiptr>(m_geometry.size() * sizeof(DrawElementsIndirectCommand)),
            nullptr, GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceIDs);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
            static_cast<GLsizeiptr>(m_geometry.size() * sizeof(GLuint)),
            instanceIDs.data(), GL_STATIC_DRAW);
}

/****************************************************************************/

void Renderer::render(const bool renderBBoxes)
{
    if (m_geometry.empty())
        return;

    return;

    core::res::materials->bind();
    core::res::instances->bind();
    core::res::meshes->bind();

    const auto* cam = core::res::cameras->getDefaultCam();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);

    GLuint textures[core::bindings::NUM_TEXT_UNITS] = {0,};


    glUseProgram(m_vertexpulling_prog);
    glBindVertexArray(m_vertexpulling_vao);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);
    for (const auto& cmd : m_drawlist) {
        // Frustum Culling
        if (!cam->inFrustum(cmd.aabb))
            continue;

        // bind textures
        if (cmd.textures[core::bindings::DIFFUSE_TEX_UNIT] != 0) {
            unsigned int unit = core::bindings::DIFFUSE_TEX_UNIT;
            GLuint tex = cmd.textures[unit];
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (cmd.textures[core::bindings::SPECULAR_TEX_UNIT] != 0) {
            unsigned int unit = core::bindings::SPECULAR_TEX_UNIT;
            GLuint tex = cmd.textures[unit];
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (cmd.textures[core::bindings::GLOSSY_TEX_UNIT] != 0) {
            unsigned int unit = core::bindings::GLOSSY_TEX_UNIT;
            GLuint tex = cmd.textures[unit];
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (cmd.textures[core::bindings::NORMAL_TEX_UNIT] != 0) {
            unsigned int unit = core::bindings::NORMAL_TEX_UNIT;
            GLuint tex = cmd.textures[unit];
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (cmd.textures[core::bindings::EMISSIVE_TEX_UNIT] != 0) {
            unsigned int unit = core::bindings::EMISSIVE_TEX_UNIT;
            GLuint tex = cmd.textures[unit];
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (cmd.textures[core::bindings::ALPHA_TEX_UNIT] != 0) {
            unsigned int unit = core::bindings::ALPHA_TEX_UNIT;
            GLuint tex = cmd.textures[unit];
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (cmd.textures[core::bindings::AMBIENT_TEX_UNIT] != 0) {
            unsigned int unit = core::bindings::AMBIENT_TEX_UNIT;
            GLuint tex = cmd.textures[unit];
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }

        glMultiDrawElementsIndirect(cmd.mode, cmd.type,
                cmd.indirect, cmd.drawcount, cmd.stride);
    }
    glBindVertexArray(0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

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

void Renderer::downsampleDepthBuffer(const OffscreenBuffer& buffer)
{
    glUseProgram(m_oc_downsample_prog);

    unsigned int width = static_cast<unsigned int>(buffer.m_width / 2);
    unsigned int height = static_cast<unsigned int>(buffer.m_height / 2);
    for (int i = 1; i < buffer.m_levels; ++i) {
        glBindImageTexture(0, buffer.m_depthbuffer, i - 1, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(1, buffer.m_depthbuffer, i, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

        glDispatchCompute((width + 15) / 16, (height + 15) / 16, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        width /= 2;
        height /= 2;
    }
}

/****************************************************************************/
