#include <cassert>
#include <algorithm>

#include <glm/gtc/type_ptr.hpp>

#include "renderer.h"
#include "framework/vars.h"
#include "core/mesh.h"
#include "core/shader_manager.h"
#include "core/mesh_manager.h"
#include "core/instance_manager.h"
#include "core/camera_manager.h"
#include "core/material_manager.h"
#include "core/shader_interface.h"
#include "core/texture_manager.h"
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
  : m_offscreen_buffer(1024, 768)
{
    //auto minsize = glm::min(m_offscreen_buffer.m_height, m_offscreen_buffer.m_width);
    //const auto levels = static_cast<int>(std::floor(std::log2(minsize)));
    const auto levels = m_offscreen_buffer.m_levels;
    LOG_INFO("levels: ", levels);

    glBindTexture(GL_TEXTURE_2D, m_hiz_tex);
    glTexStorage2D(GL_TEXTURE_2D, levels, GL_R32F, m_offscreen_buffer.m_width, m_offscreen_buffer.m_height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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

    core::res::shaders->registerShader("occlusionculling_comp", "basic/occlusionculling.comp",
            GL_COMPUTE_SHADER);
    m_oc_culling_prog = core::res::shaders->registerProgram("occlusionculling_prog",
            {"occlusionculling_comp"});


    core::res::shaders->registerShader("debug_vert", "basic/debug.vert",
            GL_VERTEX_SHADER);
    core::res::shaders->registerShader("debugtex_frag", "basic/debug_tex.frag",
            GL_FRAGMENT_SHADER);
    m_debugtex_prog = core::res::shaders->registerProgram("debugtex_prog",
            {"debug_vert", "debugtex_frag"});
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
    instanceIDs.reserve(m_geometry.size() + 1);
    // first entry in instanceIDs is the number of instances
    instanceIDs.push_back(static_cast<GLuint>(m_geometry.size()));

    std::size_t indirect = 0;
    GLenum current_type = 0;
    const core::Material* current_mat = nullptr;
    for (const auto* g : m_geometry) {
        instanceIDs.emplace_back(g->getIndex());

        const auto* mesh = g->getMesh();
        const auto* mat = g->getMaterial();
        if (mesh->type() != current_type || mat != current_mat) {
            current_type = mesh->type();
            current_mat = mat;
            m_drawlist.emplace_back(GL_TRIANGLES, mesh->type(),
                    reinterpret_cast<const GLvoid*>(indirect), 0, 0);
        }
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

    m_indirect_buffer = gl::Buffer();
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);
    //glBufferData(GL_DRAW_INDIRECT_BUFFER,
    //        static_cast<GLsizeiptr>(m_geometry.size() * sizeof(DrawElementsIndirectCommand)),
    //        nullptr, GL_STATIC_DRAW);
    glBufferStorage(GL_DRAW_INDIRECT_BUFFER,
            static_cast<GLsizeiptr>(m_geometry.size() * sizeof(DrawElementsIndirectCommand)),
            nullptr, GL_MAP_READ_BIT);

    m_bbox_indirect_buffer = gl::Buffer();
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_bbox_indirect_buffer);
    //glBufferData(GL_DRAW_INDIRECT_BUFFER,
    //        static_cast<GLsizeiptr>(m_geometry.size() * sizeof(DrawElementsIndirectCommand)),
    //        nullptr, GL_STATIC_DRAW);
    glBufferStorage(GL_DRAW_INDIRECT_BUFFER,
            static_cast<GLsizeiptr>(m_geometry.size() * sizeof(DrawElementsIndirectCommand)),
            nullptr, GL_MAP_READ_BIT);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

    m_instanceIDs = gl::Buffer();
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceIDs);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER,
            static_cast<GLsizeiptr>(instanceIDs.size() * sizeof(GLuint)),
            instanceIDs.data(), 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

/****************************************************************************/

void Renderer::render(const bool renderBBoxes, bool hiz, int level)
{
    if (m_geometry.empty())
        return;

    core::res::materials->bind();
    core::res::instances->bind();
    core::res::meshes->bind();

    auto* cam = reinterpret_cast<core::PerspectiveCamera*>(core::res::cameras->getDefaultCam());
    cam->setNear(1.0);
    cam->setFar(5000.0);
    const auto aspect_ratio_orig = cam->getAspectRatio();
    cam->setAspectRatio(1024.0 / 768.0);


    generateDrawcalls(m_offscreen_buffer);

    if (hiz) {
        level = std::max(level, 0);
        level = std::min(m_offscreen_buffer.m_levels - 1, level);
        glDisable(GL_DEPTH_TEST);
        glUseProgram(m_debugtex_prog);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(m_vertexpulling_vao);
        glBindTexture(GL_TEXTURE_2D, m_hiz_tex);
        glUniform1i(0, level);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    // bind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_offscreen_buffer.m_framebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, m_offscreen_buffer.m_width, m_offscreen_buffer.m_height);

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

    // unbind:

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_offscreen_buffer.m_framebuffer);
    glViewport(0, 0, vars.screen_width, vars.screen_height);
    if (!hiz) {
        glBlitFramebuffer(0, 0, m_offscreen_buffer.m_width, m_offscreen_buffer.m_height,
                0, 0, vars.screen_width, vars.screen_height,
                GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    cam->setAspectRatio(aspect_ratio_orig);


    //const auto* ptr = static_cast<DrawElementsIndirectCommand*>(glMapNamedBufferEXT(m_indirect_buffer, GL_READ_ONLY));
    //unsigned int num_culled = 0;
    //for (std::size_t i = 0; i < m_geometry.size(); ++i) {
    //    //LOG_INFO("count: ", ptr[i].count,
    //    //       "\ninstanceCount: ", ptr[i].instanceCount,
    //    //       "\nfirstIndex: ", ptr[i].firstIndex,
    //    //       "\nbaseVertex: ", ptr[i].baseVertex,
    //    //       "\nbaseInstance: ", ptr[i].baseInstance, "\n");
    //    if (ptr[i].count == 0)
    //        num_culled++;
    //}
    //glUnmapNamedBufferEXT(m_indirect_buffer);
    //LOG_INFO("Num culled: ", num_culled);

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
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_bbox_indirect_buffer);

    glDisable(GL_DEPTH_TEST);

    glMultiDrawElementsIndirect(GL_LINES, GL_UNSIGNED_BYTE,
            nullptr, static_cast<GLsizei>(m_geometry.size()), 0);
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

void Renderer::generateDrawcalls(const OffscreenBuffer& buffer)
{
    downsampleDepthBuffer(buffer);

    glUseProgram(m_oc_culling_prog);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_hiz_tex);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, m_indirect_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, m_instanceIDs);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, m_bbox_indirect_buffer);

    GLuint num_x = static_cast<GLuint>((m_geometry.size() + 255) / 256);
    glDispatchCompute(num_x, 1, 1);
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, 0);
}

/****************************************************************************/

void Renderer::downsampleDepthBuffer(const OffscreenBuffer& buffer)
{
    //auto minsize = glm::min(m_offscreen_buffer.m_height, m_offscreen_buffer.m_width);
    //const auto levels = static_cast<int>(std::floor(std::log2(minsize)));
    const auto levels = m_offscreen_buffer.m_levels;

    glUseProgram(m_oc_downsample_prog);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_offscreen_buffer.m_depthbuffer);

    unsigned int width = static_cast<unsigned int>(buffer.m_width);
    unsigned int height = static_cast<unsigned int>(buffer.m_height);
    for (int i = 0; i < levels; ++i) {
        //LOG_INFO("Level ", i, ": ", width, "x", height);
        if (i != 0) {
            glBindImageTexture(1, m_hiz_tex, i - 1, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        }
        glBindImageTexture(2, m_hiz_tex, i, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

        glDispatchCompute((width + 15) / 16, (height + 15) / 16, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        width = std::max(1u, width / 2);
        height = std::max(1u, height / 2);
    }
    glBindImageTexture(1, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
    glBindImageTexture(2, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
}

/****************************************************************************/

