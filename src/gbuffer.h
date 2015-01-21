#include "gl/opengl.h"

class GBuffer
{
public:
    GBuffer(int width, int height);

    void bindFramebuffer(bool saveState = true);
    void unbindFramebuffer();
    void bindTextures() const;
    int getWidth() const;
    int getHeight() const;

private:
    int                 m_width;
    int                 m_height;
    gl::Framebuffer     m_fbo;
    gl::Texture         m_depth_tex;
    gl::Texture         m_diffuse_normal;
    gl::Texture         m_specular_gloss_emissive;

    int                 m_prev_fbo;
    int                 m_prev_viewport[4];
};

