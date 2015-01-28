#include "log/log.h"
#include "core/shader_manager.h"
#include "core/shader_interface.h"
#include "gbuffer.h"
#include "framework/vars.h"

#include "smaa/AreaTex.h"
#include "smaa/SearchTex.h"

/****************************************************************************/

GBuffer::GBuffer(const int width, const int height)
  : m_width{static_cast<int>(vars.r_ssaa * static_cast<float>(width))},
    m_height{static_cast<int>(vars.r_ssaa * static_cast<float>(height))}
{
    GLint max_color_attachments;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
    LOG_INFO("max color attachments: ", max_color_attachments);

    GLint prev_fbo;
    GLint prev_tex;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_fbo);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex);

    // Setup GBuffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_gbuffer_fbo);

    glBindTexture(GL_TEXTURE_2D, m_depth_tex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F,
            m_width, m_height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D, m_depth_tex, 0);

    glBindTexture(GL_TEXTURE_2D, m_diffuse_normal);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F,
            m_width, m_height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, m_diffuse_normal, 0);

    glBindTexture(GL_TEXTURE_2D, m_specular_gloss_emissive);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F,
            m_width, m_height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
            GL_TEXTURE_2D, m_specular_gloss_emissive, 0);

    GLenum drawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, drawBuffers);

    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Framebuffer not complete");
    }

    // Setup accumulation
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_accumulate_fbo);

    glBindTexture(GL_TEXTURE_2D, m_accumulate0_tex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, m_width, m_height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, m_accumulate0_tex, 0);

    glBindTexture(GL_TEXTURE_2D, m_accumulate1_tex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, m_width, m_height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
            GL_TEXTURE_2D, m_accumulate1_tex, 0);

    glDrawBuffers(2, drawBuffers);
    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Framebuffer not complete");
    }

    // SMAA
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_smaa_fbo);

    glBindTexture(GL_TEXTURE_2D, m_edgesTex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, m_width, m_height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, m_edgesTex, 0);

    glBindTexture(GL_TEXTURE_2D, m_blendTex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, m_width, m_height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
            GL_TEXTURE_2D, m_blendTex, 0);

    glDrawBuffers(2, drawBuffers);
    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Framebuffer not complete");
    }

    glBindTexture(GL_TEXTURE_2D, m_areaTex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG8, AREATEX_WIDTH, AREATEX_HEIGHT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, AREATEX_WIDTH, AREATEX_HEIGHT,
            GL_RG, GL_UNSIGNED_BYTE, areaTexBytes);

    glBindTexture(GL_TEXTURE_2D, m_searchTex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, AREATEX_WIDTH, AREATEX_HEIGHT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, AREATEX_WIDTH, AREATEX_HEIGHT,
            GL_RED, GL_UNSIGNED_BYTE, searchTexBytes);

    core::res::shaders->registerShader("smaa_edge_detect_vert", "smaa/edgedetect.vert",
            GL_VERTEX_SHADER);
    core::res::shaders->registerShader("smaa_edge_detect_frag", "smaa/edgedetect.frag",
            GL_FRAGMENT_SHADER);
    core::res::shaders->registerShader("smaa_blending_weight_vert", "smaa/blending_weight_calculation.vert",
            GL_VERTEX_SHADER);
    core::res::shaders->registerShader("smaa_blending_weight_frag", "smaa/blending_weight_calculation.frag",
            GL_FRAGMENT_SHADER);
    core::res::shaders->registerShader("smaa_blending_neighborhood_vert", "smaa/neighborhood_blending.vert",
            GL_VERTEX_SHADER);
    core::res::shaders->registerShader("smaa_blending_neighborhood_frag", "smaa/neighborhood_blending.frag",
            GL_FRAGMENT_SHADER);
    m_edge_detect_prog = core::res::shaders->registerProgram("smaa_edge_detect_prog",
            {"smaa_edge_detect_vert", "smaa_edge_detect_frag"});
    m_blending_weight_prog = core::res::shaders->registerProgram("smaa_blending_weight_prog",
            {"smaa_blending_weight_vert", "smaa_blending_weight_frag"});
    m_blending__neighborhood_prog = core::res::shaders->registerProgram("smaa_blending_neighborhood_prog",
            {"smaa_blending_neighborhood_vert", "smaa_blending_neighborhood_vert"});

    float viewportSize[2] = {static_cast<float>(m_width), static_cast<float>(m_height)};
    glProgramUniform2fv(m_edge_detect_prog, 0, 1, viewportSize);
    glProgramUniform2fv(m_blending_weight_prog, 0, 1, viewportSize);
    glProgramUniform2fv(m_blending__neighborhood_prog, 0, 1, viewportSize);

    // blit program
    core::res::shaders->registerShader("ssq_vert", "basic/ssq.vert", GL_VERTEX_SHADER);
    core::res::shaders->registerShader("ssq_frag", "basic/ssq.frag", GL_FRAGMENT_SHADER);
    m_blit_prog = core::res::shaders->registerProgram("blit_prog", {"ssq_vert", "ssq_frag"});

    // gamma correction
    core::res::shaders->registerShader("gamma_frag", "basic/gamma.frag",
            GL_FRAGMENT_SHADER);
    m_gamma_prog = core::res::shaders->registerProgram("gamma_prog",
            {"ssq_vert", "gamma_frag"});

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(prev_fbo));
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prev_tex));

}

/****************************************************************************/

int GBuffer::getWidth() const
{
    return m_width;
}

/****************************************************************************/

int GBuffer::getHeight() const
{
    return m_height;
}

/****************************************************************************/

void GBuffer::bindFramebuffer()
{
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_prev_fbo);
    glGetIntegerv(GL_VIEWPORT, m_prev_viewport);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_gbuffer_fbo);
    glViewport(0, 0, m_width, m_height);
}

/****************************************************************************/

void GBuffer::bindForShading()
{
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_accumulate_fbo);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);
    bindTextures();
}

/****************************************************************************/

void GBuffer::unbindFramebuffer()
{
    // perform post processing steps:
    // - Gamme correction
    glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, m_accumulate0_tex);
    glUseProgram(m_blit_prog);
    glDrawArrays(GL_TRIANGLES, 0, 3);


    glDepthMask(GL_TRUE);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(m_prev_fbo));
    glViewport(m_prev_viewport[0], m_prev_viewport[1],
            m_prev_viewport[2], m_prev_viewport[3]);
}

/****************************************************************************/

void GBuffer::bindTextures() const
{
    glBindMultiTextureEXT(GL_TEXTURE0 + core::bindings::GBUFFER_DEPTH_TEX,
            GL_TEXTURE_2D, m_depth_tex);
    glBindMultiTextureEXT(GL_TEXTURE0 + core::bindings::GBUFFER_DIFFUSE_NORMAL_TEX,
            GL_TEXTURE_2D, m_diffuse_normal);
    glBindMultiTextureEXT(GL_TEXTURE0 + core::bindings::GBUFFER_SPECULAR_GLOSSY_EMISSIVE_TEX,
            GL_TEXTURE_2D, m_specular_gloss_emissive);
}

/****************************************************************************/

GLuint GBuffer::getDiffuseNormalTex() const
{
    return m_diffuse_normal;
}

/****************************************************************************/

GLuint GBuffer::getSpecGlossEmissiveTex() const
{
    return m_specular_gloss_emissive;
}

/****************************************************************************/

GLuint GBuffer::getDepthTex() const
{
    return m_depth_tex;
}

/****************************************************************************/

void GBuffer::blit()
{
    glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, m_accumulate1_tex);
    glUseProgram(m_blit_prog);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

/****************************************************************************/

