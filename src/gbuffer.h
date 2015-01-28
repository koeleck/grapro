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

    gl::Texture& getTexDepth() { return m_depth_tex; }
    gl::Texture& getTexNormal() { return m_diffuse_normal; }
    gl::Texture& getTexColor() { return m_specular_gloss_emissive; }

private:
    void performSMAA();

    int                 m_width;
    int                 m_height;
    gl::Framebuffer     m_gbuffer_fbo;
    gl::Texture         m_depth_tex;
    gl::Texture         m_diffuse_normal;
    gl::Texture         m_specular_gloss_emissive;

    gl::Framebuffer     m_accumulate_fbo;
    gl::Texture         m_accumulate0_tex;
    gl::Texture         m_accumulate1_tex;
    core::Program       m_blit_prog;
    core::Program       m_gamma_prog;

    // smaa
    gl::Framebuffer     m_smaa_fbo;
    gl::Texture         m_edgesTex;
    gl::Texture         m_blendTex;
    gl::Texture         m_areaTex;
    gl::Texture         m_searchTex;
    gl::Texture         m_stencilTex;
    core::Program       m_edge_detect_prog;
    core::Program       m_blending_weight_prog;
    core::Program       m_blending_neighborhood_prog;

    int                 m_prev_fbo;
    int                 m_prev_viewport[4];

};

