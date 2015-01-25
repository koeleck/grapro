#include "gl/opengl.h"
#include "core/instance.h"
#include "core/program.h"

class GBuffer
{
public:
    GBuffer(int width, int height);

    void bindFramebuffer(bool saveState = true);
    void bindReadFramebuffer();
    void unbindFramebuffer();
    void bindTextures() const;
    int getWidth() const;
    int getHeight() const;

    core::Program& getProg() { return m_gbuffer_prog; }

    gl::Texture& getTexDepth() { return m_depth_tex; }
    gl::Texture& getTexNormal() { return m_diffuse_normal; }
    gl::Texture& getTexColor() { return m_specular_gloss_emissive; }

private:
    int                 m_width;
    int                 m_height;
    gl::Framebuffer     m_fbo;
    gl::Texture         m_depth_tex;
    gl::Texture         m_diffuse_normal;
    gl::Texture         m_specular_gloss_emissive;

    int                 m_prev_fbo;
    int                 m_prev_viewport[4];

    core::Program       m_gbuffer_prog;
};

