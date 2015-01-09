#include <cmath>
#include "offscreen_buffer.h"
#include "log/log.h"

/****************************************************************************/

OffscreenBuffer::OffscreenBuffer(const int width, const int height)
  : m_width{width},
    m_height{height}
{
    const auto sz = std::max(m_width, m_height);
    const auto levels = 1 + static_cast<int>(std::floor(std::log2(sz)));
    m_levels = levels;

    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

    glBindTexture(GL_TEXTURE_2D, m_colorbuffer);
    glTexStorage2D(GL_TEXTURE_2D, levels, GL_RGB16F, m_width, m_height);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, m_colorbuffer, 0);

    glBindTexture(GL_TEXTURE_2D, m_depthbuffer);
    glTexStorage2D(GL_TEXTURE_2D, levels, GL_DEPTH_COMPONENT32F, m_width, m_height);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D, m_depthbuffer, 0);

    glBindTexture(GL_TEXTURE_2D, 0);

    GLenum draw_buffer = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &draw_buffer);

    auto result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (result != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Framebuffer not complete!");
        abort();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/****************************************************************************/
