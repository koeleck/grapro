#include "log/log.h"
#include "core/shader_interface.h"
#include "gbuffer.h"

/****************************************************************************/

GBuffer::GBuffer(const int width, const int height)
  : m_width{width},
    m_height{height}
{
    GLint prev_fbo;
    GLint prev_tex;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_fbo);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);

    glBindTexture(GL_TEXTURE_2D, m_depth_tex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F,
            m_width, m_height);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D, m_depth_tex, 0);

    glBindTexture(GL_TEXTURE_2D, m_diffuse_normal);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F,
            m_width, m_height);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, m_diffuse_normal, 0);

    glBindTexture(GL_TEXTURE_2D, m_specular_gloss_emissive);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F,
            m_width, m_height);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
            GL_TEXTURE_2D, m_diffuse_normal, 0);

    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Framebuffer not complete");
    }

    GLenum drawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, drawBuffers);

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

void GBuffer::bindFramebuffer(const bool saveState)
{
    if (saveState) {
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_prev_fbo);
        glGetIntegerv(GL_VIEWPORT, m_prev_viewport);
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
}

/****************************************************************************/

void GBuffer::unbindFramebuffer()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(m_prev_fbo));
    glViewport(m_prev_viewport[0], m_prev_viewport[1],
            m_prev_viewport[2], m_prev_viewport[3]);
}

/****************************************************************************/

void GBuffer::bindTextures() const
{
    glActiveTexture(GL_TEXTURE0 + core::bindings::GBUFFER_DEPTH_TEX);
    glBindTexture(GL_TEXTURE_2D, m_depth_tex);
    glActiveTexture(GL_TEXTURE0 + core::bindings::GBUFFER_DIFFUSE_NORMAL_TEX);
    glBindTexture(GL_TEXTURE_2D, m_diffuse_normal);
    glActiveTexture(GL_TEXTURE0 + core::bindings::GBUFFER_SPECULAR_GLOSSY_EMISSIVE_TEX);
    glBindTexture(GL_TEXTURE_2D, m_specular_gloss_emissive);
}

/****************************************************************************/