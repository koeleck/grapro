#include "gl/opengl.h"
#include "core/instance.h"
#include "core/program.h"

class GBuffer
{
public:
    GBuffer(int width, int height);

    void bindFramebuffer();
    void bindForShading();
    void unbindFramebuffer();
    void bindTextures() const;
    void blit();
    int getWidth() const;
    int getHeight() const;

    GLuint getDiffuseNormalTex() const;
    GLuint getSpecGlossEmissiveTex() const;
    GLuint getDepthTex() const;
    GLuint getAccumulationTex() const;

    gl::Texture& getTexDepth() { return m_depth_tex; }
    gl::Texture& getTexNormal() { return m_diffuse_normal; }
    gl::Texture& getTexColor() { return m_specular_gloss_emissive; }

private:
    int                 m_width;
    int                 m_height;
    gl::Framebuffer     m_gbuffer_fbo;
    gl::Texture         m_depth_tex;
    gl::Texture         m_diffuse_normal;
    gl::Texture         m_specular_gloss_emissive;

    gl::Framebuffer     m_accumulate_fbo;
    gl::Texture         m_accumulation;
    core::Program       m_blit_prog;

    // smaa
    gl::Framebuffer     m_smaa_fbo;
    gl::Texture         m_edgesTex;
    gl::Texture         m_blendTex;
    gl::Texture         m_areaTex;
    gl::Texture         m_searchTex;

    int                 m_prev_fbo;
    int                 m_prev_viewport[4];

};

